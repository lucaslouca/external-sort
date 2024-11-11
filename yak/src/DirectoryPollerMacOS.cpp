#ifdef __APPLE__
#include "DirectoryPollerMacOS.h"
#include "DirectoryPollerBuilder.h"
#include "logging/Logging.h"
#include "Util.h"
#include <stdlib.h> // for srand(), rand()
#include <thread>
#include <sstream>
#include <string> // for strerror()
#include <chrono>
#include <iostream>
#include <stdio.h>
#include <errno.h>  // for errno
#include <fcntl.h>  // for O_RDONLY
#include <stdio.h>  // for fprintf()
#include <stdlib.h> // for EXIT_SUCCESS
#include <string.h> // for strerror()
#include <unistd.h> // for close()
#include <filesystem>
#include <algorithm> // for set_difference()
#include <signal.h>

DirectoryPoller::DirectoryPoller(std::string name, std::string dir_to_watch, std::shared_ptr<SignalChannel> sig_channel) : AbstractPoller(name, sig_channel), m_dir_to_watch(dir_to_watch)
{
  m_last_files = list_files();
  for (const std::string &f : m_last_files)
  {
    if (should_add_file(f, true))
    {
      Logging::INFO("Adding file:'" + f + "'", m_name);
      m_file_paths.emplace_back(f);
    }
  }
}

bool DirectoryPoller::should_add_file(const std::string &file_path, bool starting_up)
{
  std::string file_name = Util::base_name(file_path);
  return (!Util::str_starts_with(file_name.c_str(), ".") && Util::str_ends_with(file_path.c_str(), ".csv"));
}

std::set<std::string> DirectoryPoller::list_files()
{
  std::set<std::string> files;
  for (const auto &entry : std::filesystem::directory_iterator(m_dir_to_watch))
  {
    files.emplace(entry.path());
  }
  return files;
}

bool DirectoryPoller::init_dir_watch()
{
  Logging::INFO("Watching '" + m_dir_to_watch + "'", m_name);

  /* A single kqueue */
  // https://www.freebsd.org/cgi/man.cgi?kqueue
  m_kq = kqueue();

  /* Two kevent structs */
  m_ke = (struct kevent *)malloc(sizeof(struct kevent));

  /* Initialise the struct for the file descriptor */
  m_fd = open(m_dir_to_watch.c_str(), O_RDONLY);
  EV_SET(m_ke, m_fd, EVFILT_VNODE, EV_ADD | EV_CLEAR, NOTE_DELETE | NOTE_RENAME | NOTE_WRITE, 0, NULL);

  /* Register for the event */
  if (kevent(m_kq, m_ke, 1, NULL, 0, NULL) < 0)
  {
    perror("kevent");
    return false;
  }

  return true;
}

PollResult DirectoryPoller::poll()
{
  std::stringstream ss;
  ss << "Polling";
  Logging::DEBUG(ss.str(), m_name);

  // Do not wait for events if we already have files
  if (!m_file_paths.empty())
  {
    std::string p = m_file_paths.back();
    m_file_paths.pop_back();
    return PollResult(p);
  }
  else
  {
    // Poll for events
    if (m_fd == -1 || m_kq == -1)
    {
      if (!init_dir_watch())
      {
        Logging::ERROR("Unable to init directory watch for '" + m_dir_to_watch + "'", m_name);
        kill(getpid(), SIGINT);
      }
    }

    memset(m_ke, 0x00, sizeof(struct kevent));

    // Camp here for event
    struct timespec timeout;
    timeout.tv_sec = 0;
    timeout.tv_nsec = 1;
    int return_value = kevent(m_kq, NULL, 0, m_ke, 1, &timeout);
    if (return_value < 0)
    {
      perror("kevent");
    }
    else if (return_value == 0)
    {
      // timeout
    }
    else
    {
      switch (m_ke->filter)
      {
      /* File descriptor event: let's examine what happened to the file */
      case EVFILT_VNODE:
        Logging::DEBUG("Events " + std::to_string(m_ke->fflags) + " on file descriptor " + std::to_string(m_ke->ident), m_name);

        if (m_ke->fflags & NOTE_DELETE)
        {
          Logging::DEBUG("The unlink() system call was called on the file referenced by the descriptor", m_name);
        }
        if (m_ke->fflags & NOTE_WRITE)
        {
          Logging::DEBUG("A write occurred on the file referenced by the descriptor", m_name);
          std::set<std::string> current = list_files();

          std::set<std::string> added;
          // In the end, the set 'added' will contain the current-m_last_files.
          std::set_difference(current.begin(), current.end(), m_last_files.begin(), m_last_files.end(), std::inserter(added, added.end()));
          for (const std::string &f : added)
          {
            if (should_add_file(f, false))
            {
              Logging::INFO("Adding file '" + f + "'", m_name);
              m_file_paths.emplace_back(f);
            }
          }

          std::set<std::string> removed;
          // In the end, the set 'removed' will contain the m_last_files-current.
          std::set_difference(m_last_files.begin(), m_last_files.end(), current.begin(), current.end(), std::inserter(removed, removed.end()));
          for (const std::string &f : removed)
          {
            Logging::INFO("File '" + f + "' removed from directory", m_name);
          }

          m_last_files = current;
        }
        if (m_ke->fflags & NOTE_EXTEND)
        {
          Logging::INFO("The file referenced by the descriptor was extended", m_name);
        }
        if (m_ke->fflags & NOTE_ATTRIB)
        {
          Logging::INFO("The file referenced by the descriptor had its attributes changed", m_name);
        }
        if (m_ke->fflags & NOTE_LINK)
        {
          Logging::INFO("The link count on the file changed", m_name);
        }
        if (m_ke->fflags & NOTE_RENAME)
        {
          Logging::INFO("The file referenced by the descriptor was renamed", m_name);
        }
        if (m_ke->fflags & NOTE_REVOKE)
        {
          Logging::INFO("Access to the file was revoked via revoke(2) or the underlying filesystem was unmounted", m_name);
        }
        break;

      /* This should never happen */
      default:
        Logging::ERROR("Unknown filter", m_name);
      }
    }

    if (m_file_paths.empty())
    {
      return PollResult("");
    }
    else
    {
      std::string p = m_file_paths.back();
      m_file_paths.pop_back();
      return PollResult(p);
    }
  }
}

AbstractPoller *DirectoryPoller::clone() const
{
  return new DirectoryPoller(*this);
}

DirectoryPollerBuilder DirectoryPoller::builder(std::string name)
{
  return DirectoryPollerBuilder(name);
}

void DirectoryPoller::clean()
{
  Logging::INFO("Cleaning up", m_name);
}

DirectoryPoller::~DirectoryPoller()
{
  if (fcntl(m_kq, F_GETFD) != -1)
  {
    close(m_kq);
  }
}
#endif