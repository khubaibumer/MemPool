#pragma once

#include "../MemPool.h"

namespace mem {

  using uint = unsigned int;

  template<typename T>
  class shared_ptr {
   public:
	shared_ptr() : ptr_(nullptr), ref_count_(new uint(0)) {}

	explicit shared_ptr(T *ptr) : ptr_(ptr), ref_count_(new uint(1)) {}

	shared_ptr(const shared_ptr &other) {
	  this->ptr_ = other.ptr_;
	  this->ref_count_ = other.ref_count_;
	  if (other.ptr_ != nullptr) {
		(*this->ref_count_)++;// Increase the Ref-Count
	  }
	}

	shared_ptr(shared_ptr &&old) noexcept {
	  this->ptr_ = old.ptr_;
	  this->ref_count_ = old.ref_count_;

	  // cleanup old
	  old.ptr_ = nullptr;
	  old.ref_count_ = nullptr;
	}

	shared_ptr &operator=(const shared_ptr &other) {
	  if (this != &other) {
		adjust_ref_count();
		this->ptr_ = other.ptr_;
		this->ref_count_ = other.ref_count_;
		if (other.ptr_ != nullptr) {
		  (*this->ref_count_)++;// Increase the Ref-Count
		}
	  }
	}

	shared_ptr &operator=(shared_ptr &&old) noexcept {
	  adjust_ref_count();
	  this->ptr_ = old.ptr_;
	  this->ref_count_ = old.ref_count_;

	  old.ref_count_ = old.ptr_ = nullptr;
	}

	[[nodiscard]] uint get_count() const { return *this->ref_count_; }

	T *get() const { return this->ptr_; }

	T *operator->() const { return this->ptr_; }

	T &operator*() const { return this->ptr_; }

	~shared_ptr() { adjust_ref_count(); }

   private:
	void adjust_ref_count() {
	  (*ref_count_)--;
	  if (*ref_count_ == 0) {
		if (ptr_ != nullptr)
		  MemPool::returnBuffer(ptr_);
		delete ref_count_;
	  }
	}

   private:
	T *ptr_ = nullptr;
	uint *ref_count_ = nullptr;
  };

  template<typename T, typename... Args>
  shared_ptr<T> make_shared(Args &&...args) {
	if (!MEM_POOL()->isRegisteredType<T>()) {
	  MEM_POOL()->registerType<T>();
	}
	auto buffer = MEM_POOL()->getBuffer<T>();
	auto ptr = new (buffer) T((args)...);
	return shared_ptr<T>(ptr);
  }
}// namespace mem
