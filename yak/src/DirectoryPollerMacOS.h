#ifdef __APPLE__

#ifndef DIRECTORY_POLLER_MAC_OS_H
#define DIRECTORY_POLLER_MAC_OS_H
#include <memory>
#include <string>
#include <vector>
#include <sys/event.h> // for kqueue() etc.
#include <set>
#include "AbstractPoller.h"
#include "PollResult.h"

class DirectoryPollerBuilder;

class DirectoryPoller : public AbstractPoller
{
public:
  DirectoryPoller(std::string name, std::string dir_to_watch, std::shared_ptr<SignalChannel> sig_channel);
  ~DirectoryPoller() override;
  AbstractPoller *clone() const override;

  friend class DirectoryPollerBuilder;
  static DirectoryPollerBuilder builder(std::string name);

private:
  PollResult poll() override;
  void clean() override;
  std::set<std::string> list_files();
  bool should_add_file(const std::string &file_path, bool starting_up);

private:
  std::string m_dir_to_watch;
  std::vector<std::string> m_file_paths;
  bool init_dir_watch();
  int m_kq = -1;
  int m_fd = -1;
  struct kevent *m_ke;
  std::set<std::string> m_last_files;
};

#endif
#endif
