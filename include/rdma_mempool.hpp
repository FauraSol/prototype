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
#include <unordered_map>
#include <numa.h>
#include <random>

#include "log.hpp"
#include "lock.hpp"
#include "rdma_conn.h"

#define USE_RDMA
#define LOCAL_NUMA 0
#define CXL_NUMA 1
#define REMOTE_NUMA 2

using std::unordered_set;
using std::unordered_map;
using std::pair;

#include <random>

#ifdef PREDICT_ENABLED
using namespace my_ml;
#endif

struct p_data_t {
  uintptr_t addr;
  uint32_t length;
  uint32_t rkey;
};

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



template <typename T, size_t BlockSize = 4096>
class RDMAMemoryPool
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
      typedef RDMAMemoryPool<U> other;
    };

    /* Member functions */
    RDMAMemoryPool() noexcept;
    RDMAMemoryPool(const RDMAMemoryPool& memoryPool) = delete;
    RDMAMemoryPool(RDMAMemoryPool&& memoryPool) noexcept;
    template <class U> RDMAMemoryPool(const RDMAMemoryPool<U>& memoryPool) noexcept;

    ~RDMAMemoryPool() noexcept;

    RDMAMemoryPool& operator=(const RDMAMemoryPool& memoryPool) = delete;
    RDMAMemoryPool& operator=(RDMAMemoryPool&& memoryPool) noexcept;

    pointer address(reference x) const noexcept;
    const_pointer address(const_reference x) const noexcept;

    // Can only allocate one object at a time. n and hint are ignored
    pointer allocate(int pred = LOCAL_NUMA);
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

    slot_pointer_ rdmaCurrentBlock_;
    slot_pointer_ rdmaCurrentSlot_;
    slot_pointer_ rdmaLastSlot_;
    slot_pointer_ rdmaFreeSlot_;
    
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
    unordered_set<pointer> rdma_numa_addrs_;
    unordered_set<ibv_mr*> rdma_mrs_;

    size_type padPointer(data_pointer_ p, size_type align) const noexcept;
    void allocateBlock(int y_pred = REMOTE_NUMA);

    static_assert(BlockSize >= 2 * sizeof(slot_type_), "BlockSize too small.");
    Mutex mutex_;

    //最大内存容量
    size_type cur_local_memory_ = 0;
    size_type max_local_memory_ = 0;
#ifdef PREDICT_ENABLED
    EvaluationQueue<uint64_t> eq;
    HoeffdingTreeClassifier clf_;

#endif
    
public:
  RNG rng;
  RDMAConnection conn;
  RDMABatch b;
};

template <typename T, size_t BlockSize>
inline typename RDMAMemoryPool<T, BlockSize>::size_type
RDMAMemoryPool<T, BlockSize>::padPointer(data_pointer_ p, size_type align)
const noexcept
{
  uintptr_t result = reinterpret_cast<uintptr_t>(p);
  return ((align - result) % align);
}



template <typename T, size_t BlockSize>
RDMAMemoryPool<T, BlockSize>::RDMAMemoryPool()
noexcept
{
  currentBlock_ = cxlCurrentBlock_ = rdmaCurrentBlock_ =  nullptr;
  currentSlot_ = cxlCurrentSlot_ = rdmaCurrentSlot_ = nullptr;
  lastSlot_ = cxlLastSlot_= rdmaLastSlot_ = nullptr;
  freeSlots_ = cxlFreeSlot_ = rdmaFreeSlot_ = nullptr;
  
  
//   conn.connect("192.168.200.51",8765);
//   DLOG_INFO("connect ok");
}

template <typename T, size_t BlockSize>
RDMAMemoryPool<T, BlockSize>::RDMAMemoryPool(RDMAMemoryPool&& memoryPool)
noexcept
{
  currentBlock_ = memoryPool.currentBlock_;
  memoryPool.currentBlock_ = nullptr;

  cxlCurrentBlock_ = memoryPool.cxlCurrentBlock_;
  memoryPool.cxlCurrentBlock_ = nullptr;

  rdmaCurrentBlock_ = memoryPool.rdmaCurrentBlock_;
  memoryPool.rdmaCurrentBlock_ = nullptr;
  
  currentSlot_ = memoryPool.currentSlot_;
  lastSlot_ = memoryPool.lastSlot_;
  freeSlots_ = memoryPool.freeSlots;

  cxlCurrentSlot_ = memoryPool.cxlCurrentSlot_;
  cxlLastSlot_ = memoryPool.cxlLastSlot_;
  cxlFreeSlot_ = memoryPool.cxlFreeSlot_;

  rdmaCurrentSlot_ = memoryPool.rdmaCurrentSlot_;
  rdmaLastSlot_ = memoryPool.rdmaLastSlot_;
  rdmaFreeSlot_ = memoryPool.rdmaFreeSlot_;
}


template <typename T, size_t BlockSize>
template<class U>
RDMAMemoryPool<T, BlockSize>::RDMAMemoryPool(const RDMAMemoryPool<U>& memoryPool)
noexcept :
RDMAMemoryPool()
{}



template <typename T, size_t BlockSize>
RDMAMemoryPool<T, BlockSize>&
RDMAMemoryPool<T, BlockSize>::operator=(RDMAMemoryPool&& memoryPool)
noexcept
{
  if (this != &memoryPool)
  {
    std::swap(currentBlock_, memoryPool.currentBlock_);
    std::swap(cxlCurrentBlock_, memoryPool.cxlCurrentBlock_);
    std::swap(rdmaCurrentBlock_, memoryPool.rdmaCurrentBlock_);
    currentSlot_ = memoryPool.currentSlot_;
    lastSlot_ = memoryPool.lastSlot_;
    freeSlots_ = memoryPool.freeSlots_;
    cxlCurrentSlot_ = memoryPool.cxlCurrentSlot_;
    cxlLastSlot_ = memoryPool.cxlLastSlot_;
    cxlFreeSlot_ = memoryPool.cxlFreeSlot_;
    rdmaCurrentSlot_ = memoryPool.rdmaCurrentSlot_;
    rdmaLastSlot_ = memoryPool.rdmaLastSlot_;
    rdmaFreeSlot_ = memoryPool.rdmaFreeSlot_;
  }
  return *this;
}



template <typename T, size_t BlockSize>
RDMAMemoryPool<T, BlockSize>::~RDMAMemoryPool()
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
  rdma_numa_addrs_.clear();
  
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
  
  for(auto mr:rdma_mrs_){
    conn.deregister_memory(mr);
  }
}



template <typename T, size_t BlockSize>
inline typename RDMAMemoryPool<T, BlockSize>::pointer
RDMAMemoryPool<T, BlockSize>::address(reference x)
const noexcept
{
  return &x;
}



template <typename T, size_t BlockSize>
inline typename RDMAMemoryPool<T, BlockSize>::const_pointer
RDMAMemoryPool<T, BlockSize>::address(const_reference x)
const noexcept
{
  return &x;
}



template <typename T, size_t BlockSize>
void
RDMAMemoryPool<T, BlockSize>::allocateBlock(int y_pred)
{
  //迁移：
  //记录currentSlot所属的Block，以Block为单元迁移
  if(y_pred == LOCAL_NUMA){
    auto ptr = numa_alloc_onnode(BlockSize,LOCAL_NUMA);
    DLOG_ASSERT(ptr != nullptr);
    data_pointer_ newBlock = reinterpret_cast<data_pointer_>(ptr);
    //DLOG("local numa allocate");
    reinterpret_cast<slot_pointer_>(newBlock)->next = currentBlock_;
    currentBlock_ = reinterpret_cast<slot_pointer_>(newBlock);
    // Pad block body to staisfy the alignment requirements for elements
    data_pointer_ body = newBlock + sizeof(slot_pointer_);
    size_type bodyPadding = padPointer(body, alignof(slot_type_));
    currentSlot_ = reinterpret_cast<slot_pointer_>(body + bodyPadding);
    lastSlot_ = reinterpret_cast<slot_pointer_>
                (newBlock + BlockSize - sizeof(slot_type_) + 1);

    cur_local_memory_ += BlockSize;
  }else if(y_pred == CXL_NUMA){
    auto ptr = numa_alloc_onnode(BlockSize,CXL_NUMA);
    DLOG_ASSERT(ptr != nullptr);
    data_pointer_ newCXLBlock = reinterpret_cast<data_pointer_>(ptr);
    //DLOG("cxl numa allocate");
    reinterpret_cast<slot_pointer_>(newCXLBlock)->next = cxlCurrentBlock_;
    cxlCurrentBlock_ = reinterpret_cast<slot_pointer_>(newCXLBlock);
    // Pad block body to staisfy the alignment requirements for elements
    data_pointer_ body = newCXLBlock + sizeof(slot_pointer_);
    size_type bodyPadding = padPointer(body, alignof(slot_type_));
    cxlCurrentSlot_ = reinterpret_cast<slot_pointer_>(body + bodyPadding);
    cxlLastSlot_ = reinterpret_cast<slot_pointer_>
                  (newCXLBlock + BlockSize - sizeof(slot_type_) + 1);
  }else if(y_pred == REMOTE_NUMA){
    //TODO
    ibv_mr* mr = conn.register_memory(BlockSize);
    DLOG_ASSERT(mr != nullptr);
    conn.prep_rpc_send(b, 1, nullptr, 0, sizeof(p_data_t));
    RDMAFuture t = conn.submit(b);
    std::vector<const void *> resp_data_ptr;
    t.get(resp_data_ptr);
    data_pointer_ newRDMABlock = reinterpret_cast<data_pointer_>(mr->addr);
    DLOG("rdma numa allocate");
    reinterpret_cast<slot_pointer_>(newRDMABlock)->next = rdmaCurrentBlock_;
    rdmaCurrentBlock_ = reinterpret_cast<slot_pointer_>(newRDMABlock);
    // Pad block body to staisfy the alignment requirements for elements
    data_pointer_ body = newRDMABlock + sizeof(slot_pointer_);
    size_type bodyPadding = padPointer(body, alignof(slot_type_));
    rdmaCurrentSlot_ = reinterpret_cast<slot_pointer_>(body + bodyPadding);
    rdmaLastSlot_ = reinterpret_cast<slot_pointer_>
                  (newRDMABlock + BlockSize - sizeof(slot_type_) + 1);
  }else{
    DLOG_FATAL("unexpected alloc place");
  }


  // Allocate space for the new block and store a pointer to the previous one
}



template <typename T, size_t BlockSize>
inline typename RDMAMemoryPool<T, BlockSize>::pointer
RDMAMemoryPool<T, BlockSize>::allocate(int pred)
{
#ifdef PREDICT_ENABLED
  int y_pred = pred;
  DLOG("y_pred = %d",y_pred);
#elif defined USE_LOCAL
 // DLOG("USE_LOCAL");
  int y_pred = LOCAL_NUMA;
#elif defined USE_CXL
  //DLOG("USE_CXL");
  int y_pred = CXL_NUMA;
#elif defined USE_RDMA
  int y_pred = REMOTE_NUMA;
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
  }else if(y_pred == CXL_NUMA){
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
  }else if(y_pred == REMOTE_NUMA){
    if(rdmaFreeSlot_!=nullptr){
        pointer result = reinterpret_cast<pointer>(rdmaFreeSlot_);
        rdmaFreeSlot_ = rdmaFreeSlot_->next;
        rdma_numa_addrs_.insert(result);
        return result; 
    }else{
        if (rdmaCurrentSlot_ >= rdmaLastSlot_)
          allocateBlock(REMOTE_NUMA);
        rdma_numa_addrs_.insert(reinterpret_cast<pointer>(rdmaCurrentSlot_));
        return reinterpret_cast<pointer>(rdmaCurrentSlot_++);
    }
    return nullptr;
  }else{
    DLOG_FATAL("unexpected alloc place");
    return nullptr;
  }
}

template <typename T, size_t BlockSize>
inline void
RDMAMemoryPool<T, BlockSize>::deallocate(pointer p, size_type n)
{
  std::lock_guard<std::mutex> lock(mutex_);
  if (p != nullptr) {
    if(local_numa_addrs_.count(p)){
      reinterpret_cast<slot_pointer_>(p)->next = freeSlots_;
      freeSlots_ = reinterpret_cast<slot_pointer_>(p);
    }else if(cxl_numa_addrs_.count(p)){
      reinterpret_cast<slot_pointer_>(p)->next = cxlFreeSlot_;
      cxlFreeSlot_ = reinterpret_cast<slot_pointer_>(p);
    }else if(rdma_numa_addrs_.count(p)){
      reinterpret_cast<slot_pointer_>(p)->next = rdmaFreeSlot_;
      rdmaFreeSlot_ = reinterpret_cast<slot_pointer_>(p);
    }else{
      DLOG_FATAL("error");
    }
  }
}

template <typename T, size_t BlockSize>
inline typename RDMAMemoryPool<T, BlockSize>::size_type
RDMAMemoryPool<T, BlockSize>::max_size()
const noexcept
{
  size_type maxBlocks = -1 / BlockSize;
  return (BlockSize - sizeof(data_pointer_)) / sizeof(slot_type_) * maxBlocks;
}



template <typename T, size_t BlockSize>
template <class U, class... Args>
inline void
RDMAMemoryPool<T, BlockSize>::construct(U* p, Args&&... args)
{
  new (p) U (std::forward<Args>(args)...);
}



template <typename T, size_t BlockSize>
template <class U>
inline void
RDMAMemoryPool<T, BlockSize>::destroy(U* p)
{
  p->~U();
}



template <typename T, size_t BlockSize>
template <class... Args>
inline typename RDMAMemoryPool<T, BlockSize>::pointer
RDMAMemoryPool<T, BlockSize>::newElement(Args&&... args)
{
#ifdef PREDICT_ENABLED
  Feat_vec_t feat_vec(F_FEATURE_NUM);
  feat_vec[F_PC] = reinterpret_cast<Feat_elem_t>(__builtin_return_address(0));
  feat_vec[F_CLIENT_ID] = static_cast<Feat_elem_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
  auto y = clf_.predict_one(feat_vec);
  pointer result = allocate(y);
  auto ret = eq.enqueue(feat_vec[0], std::move(feat_vec));
  if(ret.has_value()){
    auto v = ret.value();
    clf_.learn_one(v.feat_vec,v.is_hot_);
  }
#else
  pointer result = allocate();
#endif
  construct<value_type>(result, std::forward<Args>(args)...);
  return result;
}



template <typename T, size_t BlockSize>
inline void
RDMAMemoryPool<T, BlockSize>::deleteElement(pointer p)
{
  if (p != nullptr) {
    p->~value_type();
    deallocate(p);
  }
}

#endif // MEMORY_POOL_H