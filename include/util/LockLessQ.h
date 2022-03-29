#pragma once

#include <atomic>

template<class T>
class LockLessQ {
 public:
	LockLessQ() : head_(nullptr), tail_(nullptr), size_(0) {}

	LockLessQ(LockLessQ &) = delete;
	LockLessQ(LockLessQ &&) = delete;

	~LockLessQ() {
		while (dequeue());
	}

	void enqueue(T *elem) {
		elem->next_ = nullptr;
		T *pred = tail_.exchange(pred);
		++size_;

		if (pred == nullptr) {
			head_ = elem;
			return;
		}
		pred->next_.store(elem);
	}

	T *dequeue() {
		if (head_ == nullptr) {
			return nullptr;
		}
		--size_;

		T *elem = head_;
		T *node = head_;

		head_ = nullptr;

		if (!tail_.compare_exchange_strong(node, nullptr, std::memory_order_seq_cst, std::memory_order_seq_cst)) {
			while ((node = elem->next_.load()) == nullptr);
			head_ = node;
		}
		elem->next_ = nullptr;
		return elem;
	}

	[[maybe_unused]] std::size_t approx_size() { return size_; }

	[[maybe_unused]] T *peek() { return head_; }

	[[maybe_unused]] bool is_empty() { return head_ == nullptr; }

 private:
	T *head_;
	std::atomic<T *> tail_;
	std::atomic<std::size_t> size_;
};
