#pragma once

#include "../MemPool.h"

namespace mem {

  using uint = unsigned int;

  template<typename T>
  class unique_ptr {
   public:
	unique_ptr() : ptr_(nullptr) {}

	explicit unique_ptr(T *ptr) : ptr_(ptr) {}

	unique_ptr(const unique_ptr &) = delete;
	unique_ptr &operator=(const unique_ptr &) = delete;

	~unique_ptr() { MemPool::returnBuffer(ptr_); }

	T *operator->() const { return ptr_; }
	T &operator*() const { return *ptr_; }

	T *get() const { return ptr_; }
	explicit operator bool() const { return ptr_; }

	T *release() {
	  T *result = nullptr;
	  std::swap(result, ptr_);
	  return result;
	}

   private:
	T *ptr_;
  };

  template<typename T, typename... Args>
  unique_ptr<T> make_unique(Args &&...args) {
	if (!MEM_POOL()->isRegisteredType<T>()) {
	  MEM_POOL()->registerType<T>();
	}
	auto buffer = MEM_POOL()->getBuffer<T>();
	auto ptr = new (buffer) T((args)...);
	return unique_ptr<T>(ptr);
  }
}// namespace mem
