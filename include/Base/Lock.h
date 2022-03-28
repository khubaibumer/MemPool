#pragma once

#include <atomic>
#include <thread>
#include <string>

class SpinLock {
 public:
	SpinLock(SpinLock &) = delete;
	SpinLock(SpinLock &&) = delete;
	~SpinLock() = default;

	SpinLock() : SpinLock("GiantLock") {}

	explicit SpinLock(const std::string &name)
		: name_(name), lock_(ATOMIC_FLAG_INIT), exhaust_limit_(get_exhaust_limit()) {}

	static uint64_t get_exhaust_limit() {
	  std::thread::hardware_concurrency();
	  return 1000000;
	}

	__always_inline bool trylock() {
	  return lock_.test_and_set(std::memory_order_acquire);
	}

	bool lock() {
	  int count = 0;
	  clock_t start = clock();
	  while (((clock() - start) / CLOCKS_PER_SEC <= 10) || count < exhaust_limit_) {
		if (trylock()) {
		  return true;
		}
		++count;
	  }
	  return false;
	}

	void unlock() {
	  lock_.clear(std::memory_order_release);
	}

 private:
	std::atomic_flag lock_;
	std::string name_;
	const uint64_t exhaust_limit_;
};

