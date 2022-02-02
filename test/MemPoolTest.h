#pragma once

#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>
#include <memory>
#include "../include/MemPool.h"
#include "../include/util/OCQueue.h"

struct MemNode {
    explicit MemNode(void *ptr) : ptr_(ptr), _next(ATOMIC_VAR_INIT(nullptr))
    { }

    MemNode() = delete;

    std::atomic<MemNode *> _next;

    void *ptr_;
};

class Logger;

using ThreadVec_t  = std::vector<std::thread>;
using ThreadsVecPtr_t = std::unique_ptr<ThreadVec_t>;
using BufferData_t = OCQueue<MemNode>;
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
