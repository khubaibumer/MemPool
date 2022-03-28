#pragma once

#include <memory>
#include <mutex>

namespace base {
class ThreadInfo;

using ThreadInfoPtr_t = std::shared_ptr<ThreadInfo>;

class ThreadInfo {
 public:
	static ThreadInfoPtr_t &getInstance();

	[[nodiscard]] const std::string &getThreadName() const { return name_; }

	uint64_t getSystemTime();

	uint64_t getSystemTimeSinceLast();

	uint64_t getUserTime();

	uint64_t getUserTimeSinceLast();

	uint getOccupancy();

	[[nodiscard]] pthread_t getTid() const { return tid_; };

 private:
	ThreadInfo();

 private:
	static thread_local ThreadInfoPtr_t instance_;
	uint64_t lastSysTime_;
	uint64_t lastUsrTime_;
	std::string name_;
	const time_t creationTime_;
	const pthread_t tid_;
};
}

#define current base::ThreadInfo::getInstance()
