#include "../include/Base/ThreadInfo.h"
#include "../include/MemPool.h"

#define MEM_ACQUIRE_LOCK(obj) (obj).lock()
#define MEM_TRY_LOCK(obj) (obj).try_lock()
#define MEM_UNLOCK(obj)  (obj).unlock();
#define MEM_LOCK_DECL(obj) std::mutex MemPool::obj{}
#define MEM_LOCK_INIT(obj)

thread_local MemPoolPtr_t MemPool::instance_ = nullptr;
std::atomic<bool> MemPool::houseKeepingInProg_ = false;
PtrsCache_t MemPool::returnedMem_;
MEM_LOCK_DECL(houseKeepingLock_);

MemPoolPtr_t &MemPool::getInstance() {
    static thread_local std::once_flag flag;
    std::call_once(flag, [&]() {
        instance_.reset(new MemPool());
    });
    return instance_;
}

MemPool::MemPool()
        : volume_(defaultVolume_), myTid_(current->getTid()),
          houseKeepingCount_(0), houseKeepingDeferCount_(0), mandatoryHouseKeepingCount_(0),
          freeMemoryBlocks_(0), returnedFreeMemoryBlocks_(0), currPool_(nullptr) {

    objectMap_ = std::make_shared<ObjectMap_t>();
    if (objectMap_ == nullptr) {
        std::cerr << __func__ << " [ERROR] objectMap_ == nullptr" << std::endl;
    }
    MEM_LOCK_INIT(houseKeepingLock_);
}

MemPool::~MemPool() {
    objectMap_->clear();
    objectMap_ = nullptr;
}

bool MemPool::doHouseKeepingIfAllowed() {
#   if VERBOSE_DEBUG
    auto stats = this->stats();
    std::cout << stats << std::endl;
#   endif

    if (isUpperThresholdMet()) {
        // This means that this memory pool is at 95% capacity; but the Thread is less than 88% Occupancy
        // we need to do housekeeping to ensure proper working; we'll spin & block
        MEM_ACQUIRE_LOCK(houseKeepingLock_);

        mandatoryHouseKeepingCount_++;
    } else if (houseKeepingInProg_ || (MEM_TRY_LOCK(houseKeepingLock_) == false)) {
        // We'll try to lock this flag without spinning; if we don't get lock someone else is doing housekeeping
        ++houseKeepingDeferCount_;
        return false;
    }
    houseKeepingInProg_ = true;
    doHouseKeeping();
    houseKeepingCount_++;
    MEM_UNLOCK(houseKeepingLock_);
    houseKeepingInProg_ = false;
    return true;
}

void MemPool::setPerObjectCount(size_t _volume) {
    volume_ = _volume; //over-ride the default volume
}

bool MemPool::registerNewObject(int _id, size_t _size) {
    const auto &itr = objectMap_->find(_id);
    if (itr != objectMap_->end()) {
        std::cerr << __func__ << " [ERROR] Key already Registered!" << std::endl;
        return false;
    }

    auto pool = std::make_shared<ObjectPool_t>(volume_, _size); // create a new Pool of Objects
    objectMap_->emplace(_id, std::move(pool));
    return true;
}

bool MemPool::validatePools() const {
    bool sane = true;
    for (const auto &pool: *objectMap_) {
        if (!pool.second->validatePool()) {
            std::cerr << __func__ << " [ERROR] Pool Sanity is compromised for Key: " << pool.first << std::endl;
            sane = false;
        }
    }
    return sane;
}

void *MemPool::getBuffer(int _id) {
    const auto &itr = objectMap_->find(_id);
    if (itr == objectMap_->end()) {
        std::cerr << __func__ << " [ERROR] Invalid Key Provided" << std::endl;
    }
    currPool_ = itr->second;
    // Only try houseKeeping if Current Threads Occupancy is less than 88%
    if (isLowerThresholdMet() && (current->getOccupancy() < threadOccupancyThreshold_)) {
        // 60% pool is exhausted
        doHouseKeepingIfAllowed(); // we'll try to do this as soon as we reach 60% exhaustion; This can be deferred till 95% exhaustion
    }
    // We will start from the last index and move forward until we find a free memory block
    auto index = currPool_->index_;
    for (; index < currPool_->totalCount_ && currPool_->pool_->at(index).inUse_; ++index);
    // So either we found a free memory block or we are overshooting the max count
    if (index >= currPool_->totalCount_) { // Overshoot case
#if VERBOSE_DEBUG
        std::ostringstream ss;
        ss  << "Index: " << index
            << " Lower Threshold:"  << FromBoolToString(isLowerThresholdMet())
            << " Upper Threshold:" << FromBoolToString(isUpperThresholdMet())
            <<" ObjectMap Info: " << itr->second.str();
        std::cout << ss.str() << std::endl;
#endif
        // We are all out of Available memory
        // We are going to allocate a new memory block
        auto ptr = calloc(1, currPool_->size_);
        if (ptr == nullptr) {
            std::cerr << __func__ << " [ERROR] No Free Memory available!" << std::endl;
        }
        // We need to keep track of this newly created memory so that once it's returned we can release it
        dispatched_.emplace(ptr, std::make_pair(-1, -1));
        ++freeMemoryBlocks_;
        currPool_ = nullptr;
        return ptr;
    }

    // So `index` is within the limit, and we found an available slot
    currPool_->index_ = index; // Update the Last Access Index
    currPool_->pool_->at(index).inUse_ = true; // This is inUse now
    ++currPool_->count_; // Update the count of inUse Memory
    const auto ptr = currPool_->pool_->at(index).data_;
    dispatched_.emplace(ptr, std::make_pair(index, _id)); // Hash the pointer and save its index with _id
    if (currPool_ && !currPool_->validatePool()) {
        abort();
    }
    currPool_ = nullptr;
    return ptr;
}

void MemPool::returnBuffer(void *_ptr) {
    if (instance_ && pthread_equal(current->getTid(), instance_->myTid_) == 0) {
        // I'm returning my own buffer
        const auto &itr = instance_->dispatched_.find(_ptr); // Find the dispatched entry
        if (itr == instance_->dispatched_.end()) {
            std::cerr << __func__ << " [ERROR] itr == instance_->dispatched_.end()" << std::endl;
        }

        const auto index = itr->second.first; // Get the object Pool index
        const auto key = itr->second.second; // Get the Object Map Key

        if (index == -1 && key == -1) { // MemPool was out of Memory so we created new
            ++instance_->returnedFreeMemoryBlocks_;
            free(_ptr);
            return;
        }

        const auto &obj = instance_->objectMap_->find(key);
        if (obj != instance_->objectMap_->end()) {
            instance_->doCleanup(obj->second, index);
        }
    } else {
        returnBufferSpecial(
                _ptr); // We are not the owner of this memory; so lets transfer this pointer back to its owner thread
    }
}

void MemPool::returnBufferSpecial(void *_ptr) {
    // this means that the thread that's returning thread is not the owner of this memory
    // So now we need to somehow make this `_ptr` node empty and its control flag as false

    // we need to find the owner thread and make it use the returnBuffer(_ptr);
    // this would be so easy if instead of having raw threads I had a threadPool
    returnedMem_.push_back(
            _ptr);// We'll insert this ptr to static list; We'll iterate over this list periodically from each thread and update dispatched_
}

void MemPool::doHouseKeeping() {
    const auto size = returnedMem_.size();
    for (auto i = 0; i < size; ++i) {
        if (current->getOccupancy() > threadOccupancyThreshold_) { // Premature Return if System is in Overload
            std::cerr << __func__ << "[ERROR] current->getOccupancy() > threadOccupancyThreshold_" << std::endl;
        }
        auto ptr = returnedMem_.front();
        returnedMem_.pop_front();
        auto itr = dispatched_.find(ptr);
        if (itr != dispatched_.end()) {
            // If this is in dispatched map with a valid key that means that we should be able to use RandomAccess operator
            const auto &index = itr->second.first;
            const auto &key = itr->second.second;
            if (key == -1 && index == -1) {
                // This key means that we were out of pre-allocated memory, and we just got new memory for use
                free(ptr);
                ++returnedFreeMemoryBlocks_;
            } else {
                const auto &pool = objectMap_->find(key);
                if (pool == objectMap_->end()) {
                    // Highly unlikely scenario.
                    // This means that the pointer is in my dispatched cache but I'm unable to find this pool in my object map
                    // If this is the case we need to alert user but at the same time we need to release this memory
                    std::cerr << __func__ << " [ERROR] Unable to find " << key << " in the Object Pool for TID:"
                              << current->getTid() << std::endl;
                    free(ptr);
                } else {
                    doCleanup(pool->second, index);
                }
            }
            // Remove this from Dispatched Cache
            dispatched_.erase(itr);
        } else {
            // If I did not dispatch this pointer I need to re-insert this into the list
            returnedMem_.push_back(ptr);
        }
    }
}

void MemPool::doCleanup(ObjectPoolPtr_t &obj, size_t index) {
    memset(obj->pool_->at(index).data_, 0, obj->size_); // Reset data
    obj->pool_->at(index).inUse_ = false; // Set inUse flag to false
    --obj->count_; // Decrease the active Count
    if (obj->index_ > index) {
        obj->index_ = index; // Adjust the last Access Index
    }
}

std::string MemPool::stats(bool detailed) const {
    std::ostringstream ret;
    ret << " [ ";
    ret << " ThreadID: " << this->myTid_ << "|"
        << " MemPool size: " << this->objectMap_->size() << "|"
        << " In Use: " << this->dispatched_.size() << "|"
        << " HouseKeeping Count: " << this->houseKeepingCount_ << "|"
        << " HouseKeeping Defer Count: " << this->houseKeepingDeferCount_ << "|"
        << " Mandatory HouseKeeping Count: " << this->mandatoryHouseKeepingCount_ << "|"
        << " Free Mem Count: " << this->freeMemoryBlocks_ << "|"
        << " Returned Free Mem Count: " << this->returnedFreeMemoryBlocks_ << "|"
        << " Gross Returned Mem Count: " << returnedMem_.size();

    if (detailed) {
        for (const auto &node: *this->objectMap_) {
            ret << " { ";
            ret << " Pool ID: " << node.first << "|"
                << " Pool Size: " << node.second->pool_->size() << "|"
                << " Pool InUse Count: " << node.second->count_ << "|"
                << " Pool Node Size: " << node.second->size_;
            ret << " } ";
        }
    }
    ret << " ] \n\n";

    return ret.str();
}
