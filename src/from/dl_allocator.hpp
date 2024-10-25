#pragma once

#include <type_traits>

/**
 * DL2 allocator adapted from https://github.com/garyttierney/me3
 */

namespace from
{
namespace DLKR
{

class DLAllocationInterface
{
  public:
    virtual ~DLAllocationInterface() = default;
    virtual unsigned int GetAllocatorId() = 0;
    virtual int _unk0x10() = 0;
    virtual unsigned int &GetHeapFlags(unsigned int &out) = 0;
    virtual unsigned long long GetHeapCapacity() = 0;
    virtual unsigned long long GetHeapSize() = 0;
    virtual unsigned long long GetBackingHeapCapacity() = 0;
    virtual unsigned long long GetAllocationCount() = 0;
    virtual unsigned long long GetSizeOfAllocation(void *pAllocation) = 0;
    virtual void *AllocateMemory(unsigned long long sizeBytes) = 0;
    virtual void *AllocateAlignedMemory(unsigned long long sizeBytes,
                                        unsigned long long alignment) = 0;
    virtual void *ReallocateMemory(void *pAllocation, unsigned long long sizeBytes) = 0;
    virtual void *ReallocateAlignedMemory(void *pAllocation, unsigned long long sizeBytes,
                                          unsigned long long alignment) = 0;
    virtual void FreeMemory(void *pAllocation) = 0;
};

template <typename T> class DLAllocatorAdapter
{
  public:
    using value_type = T;
    using size_type = unsigned long long;
    using difference_type = long long;

    using propagate_on_container_move_assignment = std::true_type;
    using is_always_equal = std::false_type;

    template <typename U>
    DLAllocatorAdapter(const DLAllocatorAdapter<U> &other) noexcept : allocator(other.allocator)
    {
    }

    T *allocate(size_type count)
    {
        return reinterpret_cast<T *>(
            allocator.AllocateAlignedMemory(count * sizeof(T), alignof(T)));
    }

    void deallocate(T *pAllocation, size_type count = 0)
    {
        allocator.FreeMemory(reinterpret_cast<void *>(pAllocation));
    }

    template <typename T1, typename T2>
    friend bool operator==(const DLAllocatorAdapter<T1> &lhs,
                           const DLAllocatorAdapter<T2> &rhs) noexcept
    {
        return &lhs.allocator == &rhs.allocator;
    }

  private:
    DLAllocationInterface &allocator;
};

}
}
