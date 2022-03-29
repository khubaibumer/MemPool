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

	static uint64_t getSystemTime();

	[[maybe_unused]] uint64_t getSystemTimeSinceLast();

	static uint64_t getUserTime();

	[[maybe_unused]] uint64_t getUserTimeSinceLast();

	static uint getOccupancy();

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
