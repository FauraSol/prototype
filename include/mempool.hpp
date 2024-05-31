/*-
 * Copyright (c) 2013 Cosku Acay, http://www.coskuacay.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include <climits>
#include <cstddef>
#include <unordered_set>
#include <numa.h>
#include <random>

#include "log.hpp"
#include "lock.hpp"

#define LOCAL_NUMA 0
#define CXL_NUMA 1

using std::unordered_set;
using std::pair;

#include <random>

// #ifdef PREDICT_ENABLED
#include "ml_util.hpp"
using namespace heat_prediction;
// #endif

class RNG {
public:
    RNG()
        : rd(),
          gen(rd()),
          dis(0, 1) {
    }

    int operator()() {
        return dis(gen);
    }

private:
    std::random_device rd;
    std::mt19937 gen;
    std::uniform_int_distribution<> dis;
};

RNG rng;

template <typename T, size_t BlockSize = 4096>
class MemoryPool
{
  public:
    static const size_t data_size = sizeof(T);

    /* Member types */
    typedef T               value_type;
    typedef T*              pointer;
    typedef T&              reference;
    typedef const T*        const_pointer;
    typedef const T&        const_reference;
    typedef size_t          size_type;
    typedef ptrdiff_t       difference_type;
    typedef std::false_type propagate_on_container_copy_assignment;
    typedef std::true_type  propagate_on_container_move_assignment;
    typedef std::true_type  propagate_on_container_swap;

    template <typename U> struct rebind {
      typedef MemoryPool<U> other;
    };

    /* Member functions */
    MemoryPool() noexcept;
    MemoryPool(const MemoryPool& memoryPool) = delete;
    MemoryPool(MemoryPool&& memoryPool) noexcept;
    template <class U> MemoryPool(const MemoryPool<U>& memoryPool) noexcept;

    ~MemoryPool() noexcept;

    MemoryPool& operator=(const MemoryPool& memoryPool) = delete;
    MemoryPool& operator=(MemoryPool&& memoryPool) noexcept;

    pointer address(reference x) const noexcept;
    const_pointer address(const_reference x) const noexcept;

    // Can only allocate one object at a time. n and hint are ignored
    pointer allocate(feature_t feat={});
    void deallocate(pointer p, size_type n = 1);

    size_type max_size() const noexcept;

    template <class U, class... Args> void construct(U* p, Args&&... args);
    template <class U> void destroy(U* p);

    template <class... Args> pointer newElement(Args&&... args);
    void deleteElement(pointer p);

  private:

    union Slot_ {
      value_type element;
      Slot_* next;
    };

    typedef char* data_pointer_;
    typedef Slot_ slot_type_;
    typedef Slot_* slot_pointer_;

    slot_pointer_ cxlCurrentBlock_;
    slot_pointer_ cxlCurrentSlot_;
    slot_pointer_ cxlLastSlot_;
    slot_pointer_ cxlFreeSlot_;

    slot_pointer_ currentBlock_;
    slot_pointer_ currentSlot_;
    slot_pointer_ lastSlot_;
    slot_pointer_ freeSlots_;

    unordered_set<pointer> local_numa_addrs_;
    unordered_set<pointer> cxl_numa_addrs_; 

    size_type padPointer(data_pointer_ p, size_type align) const noexcept;
    void allocateBlock(int y_pred = LOCAL_NUMA);

    static_assert(BlockSize >= 2 * sizeof(slot_type_), "BlockSize too small.");
    Mutex mutex_;

// #ifdef PREDICT_ENABLED
    EvaluationQueue<pointer> eq;
// #endif
};

template <typename T, size_t BlockSize>
inline typename MemoryPool<T, BlockSize>::size_type
MemoryPool<T, BlockSize>::padPointer(data_pointer_ p, size_type align)
const noexcept
{
  uintptr_t result = reinterpret_cast<uintptr_t>(p);
  return ((align - result) % align);
}



template <typename T, size_t BlockSize>
MemoryPool<T, BlockSize>::MemoryPool()
noexcept
{
  currentBlock_ = cxlCurrentBlock_ =  nullptr;
  currentSlot_ = cxlCurrentSlot_ = nullptr;
  lastSlot_ = cxlLastSlot_= nullptr;
  freeSlots_ = cxlFreeSlot_ = nullptr;
}



// template <typename T, size_t BlockSize>
// MemoryPool<T, BlockSize>::MemoryPool(const MemoryPool& memoryPool)
// noexcept :
// MemoryPool()
// {}



template <typename T, size_t BlockSize>
MemoryPool<T, BlockSize>::MemoryPool(MemoryPool&& memoryPool)
noexcept
{
  currentBlock_ = memoryPool.currentBlock_;
  memoryPool.currentBlock_ = nullptr;

  cxlCurrentBlock_ = memoryPool.cxlCurrentBlock_;
  memoryPool.cxlCurrentBlock_ = nullptr;

  currentSlot_ = memoryPool.currentSlot_;
  lastSlot_ = memoryPool.lastSlot_;
  freeSlots_ = memoryPool.freeSlots;

  cxlCurrentSlot_ = memoryPool.cxlCurrentSlot_;
  cxlLastSlot_ = memoryPool.cxlLastSlot_;
  cxlFreeSlot_ = memoryPool.cxlFreeSlot_;
}


template <typename T, size_t BlockSize>
template<class U>
MemoryPool<T, BlockSize>::MemoryPool(const MemoryPool<U>& memoryPool)
noexcept :
MemoryPool()
{}



template <typename T, size_t BlockSize>
MemoryPool<T, BlockSize>&
MemoryPool<T, BlockSize>::operator=(MemoryPool&& memoryPool)
noexcept
{
  if (this != &memoryPool)
  {
    std::swap(currentBlock_, memoryPool.currentBlock_);
    std::swap(cxlCurrentBlock_, memoryPool.cxlCurrentBlock_);
    currentSlot_ = memoryPool.currentSlot_;
    lastSlot_ = memoryPool.lastSlot_;
    freeSlots_ = memoryPool.freeSlots_;
    cxlCurrentSlot_ = memoryPool.cxlCurrentSlot_;
    cxlLastSlot_ = memoryPool.cxlLastSlot_;
    cxlFreeSlot_ = memoryPool.cxlFreeSlot_;
  }
  return *this;
}



template <typename T, size_t BlockSize>
MemoryPool<T, BlockSize>::~MemoryPool()
noexcept
{
  // DLOG("size of local:%zu",local_numa_addrs_.size());
  // DLOG("size of cxl:%zu",cxl_numa_addrs_.size());
  // for(auto local_addr:local_numa_addrs_){
  //   std::cout << "local addr: " << local_addr <<" value: " << *(T*)local_addr << std::endl;
  // }
  // for(auto cxl_addr:cxl_numa_addrs_){
  //   std::cout << "cxl addr: " << cxl_addr <<" value: " << *(T*)cxl_addr << std::endl;
  // }

  local_numa_addrs_.clear();
  cxl_numa_addrs_.clear();
  slot_pointer_ curr = currentBlock_;
  while (curr != nullptr) {
    slot_pointer_ prev = curr->next;
    numa_free(reinterpret_cast<void*>(curr),BlockSize);
    curr = prev;
  }
  slot_pointer_ cxl_curr = cxlCurrentBlock_;
  while (cxl_curr != nullptr) {
    slot_pointer_ cxl_prev = cxl_curr->next;
    numa_free(reinterpret_cast<void*>(cxl_curr),BlockSize);
    cxl_curr = cxl_prev;
  }
}



template <typename T, size_t BlockSize>
inline typename MemoryPool<T, BlockSize>::pointer
MemoryPool<T, BlockSize>::address(reference x)
const noexcept
{
  return &x;
}



template <typename T, size_t BlockSize>
inline typename MemoryPool<T, BlockSize>::const_pointer
MemoryPool<T, BlockSize>::address(const_reference x)
const noexcept
{
  return &x;
}



template <typename T, size_t BlockSize>
void
MemoryPool<T, BlockSize>::allocateBlock(int y_pred)
{
  //迁移：
  //记录currentSlot所属的Block，以Block为单元迁移
  if(y_pred == LOCAL_NUMA){
    auto ptr = numa_alloc_onnode(BlockSize,LOCAL_NUMA);
    DLOG_ASSERT(ptr != nullptr);
    data_pointer_ newBlock = reinterpret_cast<data_pointer_>(ptr);
    DLOG("local numa allocate");
    reinterpret_cast<slot_pointer_>(newBlock)->next = currentBlock_;
    currentBlock_ = reinterpret_cast<slot_pointer_>(newBlock);
    // Pad block body to staisfy the alignment requirements for elements
    data_pointer_ body = newBlock + sizeof(slot_pointer_);
    size_type bodyPadding = padPointer(body, alignof(slot_type_));
    currentSlot_ = reinterpret_cast<slot_pointer_>(body + bodyPadding);
    lastSlot_ = reinterpret_cast<slot_pointer_>
                (newBlock + BlockSize - sizeof(slot_type_) + 1);
  }else{
    auto ptr = numa_alloc_onnode(BlockSize,CXL_NUMA);
    DLOG_ASSERT(ptr != nullptr);
    data_pointer_ newCXLBlock = reinterpret_cast<data_pointer_>(ptr);
    DLOG("cxl numa allocate");
    reinterpret_cast<slot_pointer_>(newCXLBlock)->next = cxlCurrentBlock_;
    cxlCurrentBlock_ = reinterpret_cast<slot_pointer_>(newCXLBlock);
    // Pad block body to staisfy the alignment requirements for elements
    data_pointer_ body = newCXLBlock + sizeof(slot_pointer_);
    size_type bodyPadding = padPointer(body, alignof(slot_type_));
    cxlCurrentSlot_ = reinterpret_cast<slot_pointer_>(body + bodyPadding);
    cxlLastSlot_ = reinterpret_cast<slot_pointer_>
                  (newCXLBlock + BlockSize - sizeof(slot_type_) + 1);
  }
  // Allocate space for the new block and store a pointer to the previous one
}



template <typename T, size_t BlockSize>
inline typename MemoryPool<T, BlockSize>::pointer
MemoryPool<T, BlockSize>::allocate(feature_t feat)
{
#ifdef PREDICT_ENABLED
  //收集特征
  //x = 
  //作出预测
  int y_pred = rng();
  DLOG("y_pred = %d",y_pred);
#elif defined USE_LOCAL
  int y_pred = LOCAL_NUMA;
#elif defined USE_CXL
  int y_pred = CXL_NUMA;
#else
  int y_pred = LOCAL_NUMA;
#endif
  std::lock_guard<std::mutex> lock(mutex_);
  if(y_pred == LOCAL_NUMA){
    if (freeSlots_ != nullptr) {
      pointer result = reinterpret_cast<pointer>(freeSlots_);
      freeSlots_ = freeSlots_->next;
      local_numa_addrs_.insert(result);
      return result;
    }
    else {
      if (currentSlot_ >= lastSlot_)
        allocateBlock(LOCAL_NUMA);
      local_numa_addrs_.insert(reinterpret_cast<pointer>(currentSlot_));
      return reinterpret_cast<pointer>(currentSlot_++);
    }
  }else{
    if(cxlFreeSlot_!=nullptr){
      pointer result = reinterpret_cast<pointer>(cxlFreeSlot_);
      cxlFreeSlot_ = cxlFreeSlot_->next;
      cxl_numa_addrs_.insert(result);
      return result;
    }else{
      if (cxlCurrentSlot_ >= cxlLastSlot_)
        allocateBlock(CXL_NUMA);
      cxl_numa_addrs_.insert(reinterpret_cast<pointer>(cxlCurrentSlot_));
      return reinterpret_cast<pointer>(cxlCurrentSlot_++);
    }
  }
}

template <typename T, size_t BlockSize>
inline void
MemoryPool<T, BlockSize>::deallocate(pointer p, size_type n)
{
  std::lock_guard<std::mutex> lock(mutex_);
  if (p != nullptr) {
    if(local_numa_addrs_.count(p)){
      reinterpret_cast<slot_pointer_>(p)->next = freeSlots_;
      freeSlots_ = reinterpret_cast<slot_pointer_>(p);
    }else if(cxl_numa_addrs_.count(p)){
      reinterpret_cast<slot_pointer_>(p)->next = cxlFreeSlot_;
      cxlFreeSlot_ = reinterpret_cast<slot_pointer_>(p);
    }else{
      DLOG_FATAL("error");
    }
  }
}

template <typename T, size_t BlockSize>
inline typename MemoryPool<T, BlockSize>::size_type
MemoryPool<T, BlockSize>::max_size()
const noexcept
{
  size_type maxBlocks = -1 / BlockSize;
  return (BlockSize - sizeof(data_pointer_)) / sizeof(slot_type_) * maxBlocks;
}



template <typename T, size_t BlockSize>
template <class U, class... Args>
inline void
MemoryPool<T, BlockSize>::construct(U* p, Args&&... args)
{
  new (p) U (std::forward<Args>(args)...);
}



template <typename T, size_t BlockSize>
template <class U>
inline void
MemoryPool<T, BlockSize>::destroy(U* p)
{
  p->~U();
}



template <typename T, size_t BlockSize>
template <class... Args>
inline typename MemoryPool<T, BlockSize>::pointer
MemoryPool<T, BlockSize>::newElement(Args&&... args)
{
  //收集特征
  //x = 
  //作出预测
#ifdef PREDICT_ENABLED
  feature_t feat;
  feat.pc_ = __builtin_retuen_address(0);
  feat.client_id_ = std::hash<std::thread::id>{}(std::this_thread::get_id());
  pointer result = allocate(feat);
#else
  pointer result = allocate();
#endif
  construct<value_type>(result, std::forward<Args>(args)...);
  return result;
}



template <typename T, size_t BlockSize>
inline void
MemoryPool<T, BlockSize>::deleteElement(pointer p)
{
  if (p != nullptr) {
    p->~value_type();
    deallocate(p);
  }
}

#endif // MEMORY_POOL_H