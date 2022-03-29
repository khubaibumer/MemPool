#include <unistd.h>
#include <iostream>
#include "../include/Base/ThreadInfo.h"
#include "MemPoolTest.h"

BufferDataPtr_t MemPoolTest::dataQ_ = nullptr;

MemPoolTest::MemPoolTest(int threadCount)
	: workerThreads_(std::make_unique<ThreadVec_t>()) {
	threadCount_ = threadCount;
	if (workerThreads_ == nullptr) {
		std::cerr << __func__ << "[ERROR] workerThreads_ == nullptr" << std::endl;
	}
	dataQ_ = std::make_shared<BufferData_t>();
}

void MemPoolTest::runTest() {
	try {
		procTid_ = std::thread(&MemPoolTest::processorThread);
		for (auto i = 0; i < 1; i++) {
			auto worker = std::thread(&MemPoolTest::workerRoutine);
			workerThreads_->push_back(std::move(worker));
		}
	} catch (std::exception &e) {
		std::cout << "Exception: " << e.what() << std::endl;
	}
}

[[noreturn]] void MemPoolTest::processorThread() {
	while (true) {
		if (!dataQ_->is_empty()) {
			auto node = dataQ_->dequeue();
			if (node != nullptr) {
				MemPool::returnBuffer(node->ptr_);
				delete node;
			}
		}
		usleep(10);
	}
}

[[noreturn]] void MemPoolTest::workerRoutine() {
	auto initialSz = 1024;

	MEM_POOL()->registerType<int>();
	MEM_POOL()->registerType<double>();
	MEM_POOL()->registerType<std::string>();
	MEM_POOL()->registerType<ThreadVec_t>();
	MEM_POOL()->registerType<ThreadsVecPtr_t>();
	MEM_POOL()->registerType<BufferData_t>();
	MEM_POOL()->registerType<BufferDataPtr_t>();

	for (auto i = 0; i < 10; i++) {
		auto objSz = ((initialSz + i) % 4 == 0 ? initialSz + i : initialSz);
		initialSz = objSz;
		if (!MEM_POOL()->registerNewObject(i, initialSz)) {
			std::cerr << __func__ << " [ERROR] Unable to registerNewObject" << std::endl;
		}
	}

	uint64_t counter = 0;
	while (true) {
		auto ptr = MEM_POOL()->getBuffer<int>();
		auto ptr1 = MEM_POOL()->getBuffer<double>();
		auto ptr2 = MEM_POOL()->getBuffer<std::string>();
		auto ptr3 = MEM_POOL()->getBuffer<ThreadVec_t>();
		auto ptr4 = MEM_POOL()->getBuffer<ThreadsVecPtr_t>();
		if (ptr == nullptr || ptr1 == nullptr || ptr2 == nullptr || ptr3 == nullptr || ptr4 == nullptr) {
			std::cerr << "Failure!" << std::endl;
		}

		usleep(100);

		if (counter % 5000 == 0) {
			std::cout << current->getTid() << " :" << MEM_POOL()->stats(true) << std::endl;
			MEM_POOL()->validatePools();
		}

		if (counter % 50 == 0) {
			MemPool::returnBuffer(ptr);
			MemPool::returnBuffer(ptr1);
			MemPool::returnBuffer(ptr2);
			MemPool::returnBuffer(ptr3);
			MemPool::returnBuffer(ptr4);
		} else {
			sendToInternalQ(ptr);
			sendToInternalQ(ptr1);
			sendToInternalQ(ptr2);
			sendToInternalQ(ptr3);
			sendToInternalQ(ptr4);
		}

		counter++;
	}
}

void MemPoolTest::sendToInternalQ(void *sptr) {
	dataQ_->enqueue(new PointerNode(sptr));
}

void MemPoolTest::stopTest() {
	std::cout << "Stopping Test Suite..." << std::endl;
	std::for_each(workerThreads_->begin(), workerThreads_->end(), [&](auto &worker) {
		pthread_cancel(worker.native_handle());
	});
	pthread_cancel(static_cast<unsigned long>(procTid_.native_handle()));
	std::cout << "Test Suite Complete...!" << std::endl;
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) {
	MemPoolTest test(4);
	test.runTest();
	char x;
	while (true) {
		std::cin >> x;
		if (x == 'q') {
			test.stopTest();
			sleep(30);
			sleep(3);
			exit(0);
		}
	}
}
