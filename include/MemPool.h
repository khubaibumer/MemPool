#pragma once

#include <memory>
#include <thread>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <deque>
#include <list>
#include <atomic>
#include <mutex>
#include <unordered_set>
#include <sstream>
#include "Base/Limits.h"
#include "Base/Lock.h"
#include "Base/Constructs.h"

class MemPool;
using MemPoolPtr_t = std::shared_ptr<MemPool>;
using ObjectPoolPtr_t = std::shared_ptr<ObjectPool_t>;
using ObjectMap_t = std::unordered_map<uint64_t, ObjectPoolPtr_t>;
using ObjectMapPtr_t = std::shared_ptr<ObjectMap_t>;
using IndexKeyPair_t = std::pair<int, int>;
using PtrsCache_t = std::list<void *>;

class MemPool {
 public:
  static MemPoolPtr_t &getInstance();

  /// @brief Set the Volume of Memory Pool
  /// @param _volume: volume of Pool
  /// @returns void
  void setPerObjectCount(size_t _volume);

  /// @brief Should only be called once for each unique object type
  /// @param _id: ID of Object
  /// @param _size: Size of Each Object
  /// @returns true if object type is registered successfully
  bool registerNewObject(int _id, size_t _size);

  /// @brief Should only be called once for each unique object type passed as Template Argument
  /// @returns true if object type is registered successfully
  template<typename T>
  __always_inline bool registerType() { return registerNewObject(typeid(T).hash_code(), sizeof(T)); }

  /// @brief Check if Type T is already registered
  /// @returns true if T is registered or false otherwise
  template<typename T>
  __always_inline bool isRegisteredType() {
	const auto &itr = objectMap_->find(typeid(T).hash_code());
	return (itr != objectMap_->end());
  }

  /// @brief  To get a buffer of given type
  /// @param _id: ID of Object
  /// @returns: a pre-allocated memory (nullptr if the key is invalid or due to mem exhaustion)
  void *getBuffer(int _id);

  /// @brief  To get buffer of a required type
  /// @param _id: Key of Object
  /// @returns: a pre-allocated memory of specified type
  template<typename T>
  __always_inline T *getBuffer(int _id) { return (T *)(getBuffer(_id)); }

  /// @brief  To get buffer of a required type
  /// @returns: a pre-allocated memory of specified type
  template<typename T>
  __always_inline T *getBuffer() { return getBuffer<T>(typeid(T).hash_code()); }

  /// @brief  To return the buffer back to MemPool
  /// @param _ptr: Pointer to return
  /// @returns void
  static void returnBuffer(void *_ptr);

  /// @brief  To get current Memory Pool Stats
  /// @param detailed: specify true if we need detailed stats for the MemPool
  /// @returns Stats for the Current Thread's Memory Pool
  [[nodiscard]] std::string stats(bool detailed = false) const;

  /// @brief To check the sanity of All Available Memory Pools
  /// @returns TRUE if Memory Pools are sane or FALSE if one of them is Overflowed
  bool validatePools() const;

  // CTR/DTR
  MemPool();

  ~MemPool();

 private:
  /// @brief Return the Pointer for some others thread's MemPool
  /// @param _ptr: Pointer to Return
  /// @returns void
  static void returnBufferSpecial(void *_ptr);

  /// @brief This function checks the health of memory pool. If LowerThreshold/UpperThreshold is met and system is not overloaded it performs a HouseKeeping
  /// @note This Routine can take some time so its very important that System is not overloaded During its execution
  /// @note Can return prematurely if system goes to OverLoad Conditions
  /// @returns TRUE if HouseKeeping is Allowed or FALSE otherwise
  bool doHouseKeepingIfAllowed();

  /// @brief This function checks the health of memory pool. If LowerThreshold/UpperThreshold is met and system is not overloaded it performs a HouseKeeping
  /// @note This Routine can take some time so its very important that System is not overloaded During its execution
  /// @note Can return prematurely if system goes to OverLoad Conditions
  void doHouseKeeping();

  /// @brief Checks if the current Mem Pool has met the UpperThreshold
  /// @returns TRUE if UpperThreshold is Met
  bool isUpperThresholdMet() const { return (currPool_->count_ >= (currPool_->totalCount_ * upperThreshold_)); }

  /// @brief Checks if the current Mem Pool has met the LowerThreshold
  /// @returns TRUE if Lower Threshold is Met
  bool isLowerThresholdMet() { return (currPool_->count_ >= (currPool_->totalCount_ * lowerThreshold_)); }

  static void doCleanup(ObjectPoolPtr_t &obj, size_t index);

 private:
  static thread_local MemPoolPtr_t instance_;

  ObjectMapPtr_t objectMap_;

  //                  ptr,    index, key
  std::unordered_map<void *, IndexKeyPair_t> dispatched_;

  static PtrsCache_t returnedMem_;

  size_t volume_;

  const pthread_t myTid_;

  static std::atomic<bool> houseKeepingInProg_;

  static SpinLock houseKeepingLock_;

  uint64_t houseKeepingCount_;

  uint64_t houseKeepingDeferCount_;

  uint64_t mandatoryHouseKeepingCount_;

  uint64_t freeMemoryBlocks_;

  uint64_t returnedFreeMemoryBlocks_;

  ObjectPoolPtr_t currPool_;
};

#define MEM_POOL() MemPool::getInstance()
