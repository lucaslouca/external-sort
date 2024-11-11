#ifdef __linux__
#include "DirectoryPoller.h"
#include "Util.h"
#include <stdlib.h> // for srand(), rand()
#include <thread>
#include <sstream>
#include <string> // for strerror()
#include <chrono>
#include <iostream>
#include <sys/select.h>
#include <sys/inotify.h>
#include <sys/types.h>
#include <unistd.h> // for read()
#include <filesystem>
#include <iostream>
#include <signal.h>
#include "logging/Logging.h"
#include "DirectoryPollerBuilder.h"

#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_LEN (1024 * (EVENT_SIZE + 16))

DirectoryPoller::DirectoryPoller(std::string name, std::string dir_to_watch, std::shared_ptr<SignalChannel> sig_channel) : AbstractPoller(name, sig_channel), m_dir_to_watch(dir_to_watch)
{
  m_last_files = list_files();
  for (const std::string &f : m_last_files)
  {
    if (should_add_file(f, true))
    {
      Logging::INFO("Adding file '" + f + "'", m_name);
      m_file_paths.emplace_back(f);
    }
  }
}

bool DirectoryPoller::init_dir_watch()
{
  Logging::INFO("Watching '" + m_dir_to_watch + "'", m_name);

  m_fd = inotify_init();
  if (m_fd < 0)
  {
    perror("inotify_init");
  }

  m_wd = inotify_add_watch(m_fd, m_dir_to_watch.c_str(), IN_CREATE | IN_MOVE | IN_CLOSE);
}

bool DirectoryPoller::should_add_file(const std::string &file_path, bool starting_up)
{
  return (starting_up || !Util::str_ends_with(file_path.c_str(), "_inprogress")) && !Util::str_ends_with(file_path.c_str(), "_done");
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

bool DirectoryPoller::check_and_add_file(const char *file_name, const std::string &type)
{
  std::string file_path(m_dir_to_watch);
  file_path += "/";
  file_path += file_name;

  if (should_add_file(file_path, false))
  {
    m_file_paths.emplace_back(file_path);
    std::string s("The file ");
    s += file_path;
    s += " was " << type;
    Logging::INFO(s, m_name);
  }
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

  if (m_fd == -1 || m_wd == -1)
  {
    if (!init_dir_watch())
    {
      Logging::ERROR("Unable to init directory watch for '" + m_dir_to_watch + "'", m_name);
      kill(getpid(), SIGINT);
    }
  }

  // Non-blocking
  int return_value;
  fd_set descriptors;
  struct timeval time_to_wait = {1, 0};
  FD_ZERO(&descriptors);
  FD_SET(m_fd, &descriptors);

  return_value = select(m_fd + 1, &descriptors, NULL, NULL, &time_to_wait);
  if (return_value < 0)
  {
    perror("select");
  }
  else if (!return_value)
  {
    // timeout
  }
  else if (FD_ISSET(m_fd, &descriptors))
  {
    int length, i = 0;
    char buffer[BUF_LEN];

    length = read(m_fd, buffer, BUF_LEN); // there was data to read
    if (length < 0)
    {
      perror("read");
    }

    while (i < length)
    {
      struct inotify_event *event = (struct inotify_event *)&buffer[i];
      if (event->len)
      {
        if (event->mask & IN_CREATE)
        {
          if (event->mask & IN_ISDIR)
          {
            std::string s("The directory ");
            s += event->name;
            s += " was created.";
            Logging::INFO(s, m_name);
          }
          else
          {
            check_and_add_file(event->name, "created");
          }
        }
        else if (event->mask & (IN_CLOSE))
        {
          if (event->mask & IN_ISDIR)
          {
            std::string s("The directory ");
            s += event->name;
            s += " was closed.";
            Logging::INFO(s, m_name);
          }
        }
        else if (event->mask & (IN_MOVE_TO))
        {
          if (event->mask & IN_ISDIR)
          {
            std::string s("The directory ");
            s += event->name;
            s += " was moved to.";
            Logging::INFO(s, m_name);
          }
          else
          {
            check_and_add_file(event->name, "moved to");
          }
        }
      }
      i += EVENT_SIZE + event->len;
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

DirectoryPollerBuilder DirectoryPoller::builder(std::string name)
{
  return DirectoryPollerBuilder(name);
}

AbstractPoller *DirectoryPoller::clone() const
{
  return new DirectoryPoller(*this);
}

void DirectoryPoller::clean()
{
}

DirectoryPoller::~DirectoryPoller()
{
  (void)inotify_rm_watch(m_fd, m_wd);
  (void)close(m_fd);
}
#endif