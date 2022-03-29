#include "../../include/Base/ThreadInfo.h"
#include <sys/resource.h>
#include <unistd.h>
#include <syscall.h>

#define GET_TID() syscall(SYS_gettid)

namespace base {

  thread_local ThreadInfoPtr_t ThreadInfo::instance_ = nullptr;

  ThreadInfo::ThreadInfo() : creationTime_(time(nullptr)), lastSysTime_(0), lastUsrTime_(0), tid_(GET_TID()) {}

  ThreadInfoPtr_t &ThreadInfo::getInstance() {
	static thread_local std::once_flag flag_;
	std::call_once(flag_, [&]() {
	  instance_.reset(new ThreadInfo());
	});
	return instance_;
  }

  uint64_t ThreadInfo::getSystemTime() {
	struct rusage thStats {};
	if (getrusage(RUSAGE_THREAD, &thStats) == 0) {
	  return (thStats.ru_stime.tv_sec * (uint64_t)1000) + (thStats.ru_stime.tv_usec / 1000);
	}
	return 0;
  }

  [[maybe_unused]] uint64_t ThreadInfo::getSystemTimeSinceLast() {
	const auto curr = getSystemTime();
	if (lastSysTime_ == 0) {
	  lastSysTime_ = curr;
	  return 0;
	}
	const auto diff = curr - lastSysTime_;
	lastSysTime_ = curr;
	return diff;
  }

  uint64_t ThreadInfo::getUserTime() {
	struct rusage thStats {};
	if (getrusage(RUSAGE_THREAD, &thStats) == 0) {
	  return (thStats.ru_utime.tv_sec * (uint64_t)1000) + (thStats.ru_utime.tv_usec / 1000);
	}
	return 0;
  }

  [[maybe_unused]] uint64_t ThreadInfo::getUserTimeSinceLast() {
	const auto curr = getUserTime();
	if (lastUsrTime_ == 0) {
	  lastUsrTime_ = curr;
	  return 0;
	}
	const auto diff = curr - lastUsrTime_;
	lastUsrTime_ = curr;
	return diff;
  }

  uint ThreadInfo::getOccupancy() {
	/* I need to calculate this Thread's occupancy here */
	return 0;
  }
}

