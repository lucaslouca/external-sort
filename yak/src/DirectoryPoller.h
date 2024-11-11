#ifdef __linux__

#ifndef DIRECTORY_POLLER_H
#define DIRECTORY_POLLER_H

#include <memory>
#include <string>
#include <vector>
#include <set>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include "AbstractPoller.h"
#include "impl/PollResult.h"

class DirectoryPollerBuilder;

class DirectoryPoller : public AbstractPoller
{
private:
  PollResult poll() override;
  void clean() override;
  bool init_dir_watch();
  std::set<std::string> list_files();
  bool should_add_file(const std::string &file_path, bool starting_up);
  bool check_and_add_file(const char *file_name, const std::string &type);

private:
  std::string m_dir_to_watch;
  std::vector<std::string> m_file_paths;
  int m_fd = -1;
  int m_wd = -1;
  std::set<std::string> m_last_files;
  std::atomic<bool> *m_data_available;
  std::condition_variable *m_queue_cv;
  std::mutex *m_queue_cv_mutex;

public:
  DirectoryPoller(std::string name, std::string dir_to_watch, std::shared_ptr<SignalChannel> sig_channel);
  ~DirectoryPoller() override;
  AbstractPoller *clone() const override;

  friend class DirectoryPollerBuilder;
  static DirectoryPollerBuilder builder(std::string name);
};
#endif
#endif
