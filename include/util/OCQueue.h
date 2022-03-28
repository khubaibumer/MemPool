// OCQueue: threadsafe lockless queue
// Works for any number of producers (enqueue) but only one consumer (dequeue)

#ifndef __OCQUEUE_H
#define __OCQUEUE_H

#include <atomic>

template<class T>
class OCQueue {
 public:
	OCQueue() {
		_head = nullptr;
		_tail.store(nullptr);
		_size.store(0);
	}
	~OCQueue() {
		// discard all queue elements.  This will
		// likely memory leak if there's anything there.
		while (dequeue());
	}
	void enqueue(T *e) {
		e->_next = nullptr;
		T *predecessor = e;
		predecessor = _tail.exchange(predecessor);
		_size++;
		// if the queue's former tail pointer was null, the queue was empty,
		// and I am now the head of the queue.  Otherwise, I just added myself
		// to the tail of the queue, and I have to update the previous node's
		// next pointer (which must have been nullptr).
		if (predecessor == nullptr) {
			// I am now the head of the queue!
			_head = e;
			return;
		}
		predecessor->_next.store(e);
		return;
	}
	T *dequeue() {
		if (_head == nullptr) {
			return nullptr;
		}
		_size--;
		// get the head element, we'll return it below
		T *e = _head;
		T *node = _head;    // parameter for exchange_strong
		// in case the swap we're about to do succeeds, the queue must appear to be empty
		// so other threads can populate the "_head" entry
		_head = nullptr;
		if (!_tail.compare_exchange_strong(node, nullptr, std::memory_order_seq_cst, std::memory_order_seq_cst)) {
			// if the swap failed, it means something was on the queue behind me.
			// Update the _head to the next element.
			// However, I must first make sure that other thread has updated my next pointer.
			while ((node = e->_next.load()) == nullptr);    // spin until it has
			_head = node;
		}
		// else both the head and tail are nullptr, queue is empty
		e->_next = nullptr;
		return e;
	}
	// estimated_size is only an estimate: threads might be enqueueing/dequeueing at the same time
	unsigned estimated_size(void) {
		return _size;
	}
	T *peek(void) {
		return _head;
	}
	bool empty(void) {
		return (_head == nullptr);
	}

 private:
	T *_head;
	std::atomic<T *> _tail;
	std::atomic<unsigned> _size;
};

#endif
