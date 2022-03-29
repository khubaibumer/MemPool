#pragma once

#include <vector>
#include <memory>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <cstring>
#include <atomic>
#include "../util/LockLessQ.h"

#define GUARD_BYTES_COUNT 5
#define CACHE_LINE_SIZE 64
static const uint8_t gTestGuard[GUARD_BYTES_COUNT] = {0, 0, 0, 0, 0};

typedef struct PoolNode {
  bool inUse_; // Control Variable
  void *data_; // Data Chunk
  PoolNode() = delete;

  explicit PoolNode(bool inUse, void *data)
	  : inUse_(inUse), data_(data) {}
} PoolNode_t;

using PoolNodePtr_t = std::unique_ptr<PoolNode_t>;
using PoolVec_t = std::vector<PoolNodePtr_t>;
using PoolVecPtr_t = std::unique_ptr<PoolVec_t>;

typedef struct ObjectPool {
  size_t totalCount_ {}; // Total Number of Objects Available
  size_t size_; // Object Size
  size_t count_; // Object Size
  size_t index_; // Last accessed Index
  void *chunkHead_;
  uint8_t *guard_;
  PoolVecPtr_t pool_; // Vector of Data Nodes

  [[nodiscard]] std::string str() const {
	std::ostringstream ss;
	ss << " [ " << std::endl;
	ss << "\ttotalCount_: " << totalCount_ << std::endl;
	ss << "\tcount_: " << count_ << std::endl;
	ss << "\tsize_: " << size_ << std::endl;
	ss << "\tindex_: " << index_ << std::endl;
	ss << "]" << std::endl;
	return ss.str();
  }

  explicit ObjectPool(size_t volume, size_t size) {
	if (size % 64 != 0) {
	  size += 64;
	  size = (size - (sizeof(int) * 2));
	}
	totalCount_ = volume;
	size_ = size;
	count_ = 0;
	index_ = 0;
	pool_ = std::make_unique<PoolVec_t>();
	chunkHead_ = calloc(volume + GUARD_BYTES_COUNT, size);
	if (chunkHead_ == nullptr) {
	  std::cerr << __func__ << " [ERROR] chunkHead_ == nullptr" << std::endl;
	}
	int i = 0;
	for (; i < totalCount_; i++) {
	  void *ref = ((uint8_t *)chunkHead_ + (size * i));
	  pool_->emplace_back(std::make_unique<PoolNode_t>(false, ref));
	}
	guard_ = ((uint8_t *)chunkHead_ + (size * i));
  }

  ObjectPool() = delete;

  ~ObjectPool() {
	if (chunkHead_) {
	  free(chunkHead_);
	  chunkHead_ = nullptr;
	}
  }

  [[nodiscard]] bool validatePool() const {
	if (memcmp(guard_, gTestGuard, GUARD_BYTES_COUNT) != 0) {
	  std::cerr << __func__ << " Memory Corruption Detected (Overflow). Re-run with ASan recommended!"
				<< std::endl;
	  return false;
	}
	return true;
  }
} ObjectPool_t;

struct PointerNode {
  void *ptr_;
  std::atomic<PointerNode *> next_;

  explicit PointerNode(void *ptr)
	  : ptr_(ptr), next_(nullptr) {}

  PointerNode() = delete;
};
