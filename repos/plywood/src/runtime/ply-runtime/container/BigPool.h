/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>

namespace ply {

// FIXME: Implement a 32-bit friendly BigPool, especially for crowbar::Tokenizer
PLY_STATIC_ASSERT(PLY_PTR_SIZE == 8);

struct BaseBigPool {
    char* base = nullptr;
    uptr numReservedBytes = 0;
    uptr numCommittedBytes = 0;

    static constexpr uptr DefaultNumReservedBytes = 1024 * 1024 * 1024;

    BaseBigPool(uptr numReservedBytes = DefaultNumReservedBytes);
    ~BaseBigPool();
    void commitPages(uptr newTotalBytes);
};

template <typename T = char>
class BigPool : protected BaseBigPool {
private:
    uptr numItems_ = 0;

public:
    PLY_INLINE BigPool(uptr numReservedBytes = DefaultNumReservedBytes)
        : BaseBigPool{numReservedBytes} {
    }
    PLY_INLINE uptr numItems() const {
        return this->numItems_;
    }
    PLY_INLINE const T& operator[](uptr idx) const {
        PLY_ASSERT(idx < this->numItems_);
        return ((const T*) this->base)[idx];
    }
    PLY_INLINE T& operator[](uptr idx) {
        PLY_ASSERT(idx < this->numItems_);
        return ((T*) this->base)[idx];
    }
    PLY_INLINE const T* end() const {
        return ((const T*) this->base) + this->numItems_;
    }
    PLY_INLINE T* get(uptr idx) const {
        PLY_ASSERT(idx < this->numItems_);
        return ((T*) this->base) + idx;
    }
    PLY_INLINE T& back(sptr ofs = -1) const {
        PLY_ASSERT(ofs < 0 && uptr(-ofs) <= this->numItems_);
        return ((T*) this->base)[this->numItems_ + ofs];
    }
    PLY_INLINE T* beginWrite(uptr maxNumItems = 1) {
        uptr newTotalBytes = sizeof(T) * (this->numItems_ + maxNumItems);
        if (newTotalBytes > this->numCommittedBytes) {
            this->commitPages(newTotalBytes);
        }
        return ((T*) this->base) + this->numItems_;
    }
    PLY_INLINE void endWrite(uptr numItems = 1) {
        this->numItems_ += numItems;
        PLY_ASSERT(sizeof(T) * this->numItems_ <= this->numCommittedBytes);
    }
    PLY_INLINE T* alloc(uptr numItems = 1) {
        T* result = beginWrite(numItems);
        endWrite(numItems);
        return result;
    }
    PLY_INLINE T& append(T&& item) {
        return *new (this->alloc()) T{std::move(item)};
    }
    PLY_INLINE T& append(const T& item) {
        return *new (this->alloc()) T{item};
    }
    template <typename... Args>
    PLY_INLINE T& append(Args&&... args) {
        return *new (this->alloc()) T{std::forward<Args>(args)...};
    }
};

} // namespace ply
