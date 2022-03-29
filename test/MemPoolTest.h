#pragma once

#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>
#include <memory>
#include "../include/MemPool.h"

using ThreadVec_t = std::vector<std::thread>;
using ThreadsVecPtr_t = std::unique_ptr<ThreadVec_t>;
using BufferData_t = LockLessQ<PointerNode>;
using BufferDataPtr_t = std::shared_ptr<BufferData_t>;

class MemPoolTest {
 public:
	MemPoolTest() = delete;

	explicit MemPoolTest(int threadCount);

	virtual ~MemPoolTest() = default;

	void runTest();

	void stopTest();

 private:
	static void sendToInternalQ(void *sptr);

	[[noreturn]] static void processorThread();

	[[noreturn]] static void workerRoutine();

 private:
	size_t threadCount_;
	std::thread procTid_;
	ThreadsVecPtr_t workerThreads_;
	static BufferDataPtr_t dataQ_;
};
