/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/container/ArrayView.h>
#include <ply-runtime/container/details/BaseArray.h>
#include <ply-runtime/memory/Heap.h>

namespace ply {

//------------------------------------------------------------------------------------------------
/*!
An `Array` object owns a single block of memory, allocated on the heap, that contains an array of
typed items. The items are destructed and freed when the `Array` is destroyed.

When an `Array` is `const`, the items it owns are immutable.

The array items must be relocatable; that is, it must always be possible to move items to a new
address by calling `memmove` on the memory that contains them. Most Plywood containers, including
`Array`, `Owned`, `HashMap` and `BTree`, are relocatable, so it's OK to create an `Array` of such
types or structures of such types.
 */
template <typename T_>
class Array {
public:
    using T = T_;

private:
    T* items;
    u32 numItems_;
    u32 allocated;

    // ply::Arrays of C-style arrays such as Array<int[2]> are not allowed.
    // One reason is that placement new doesn't work (to copy/move/construct new items).
    // Another reason is that it confuses the .natvis debug visualizer in Visual Studio.
    // Make an Array of FixedArray<> instead.
    PLY_STATIC_ASSERT(!std::is_array<T>::value);

    // Underlying type must be relocatable.
    // For now, structs must manually "opt out" of relocatability.
    // That includes structs containing those structs!
    PLY_STATIC_ASSERT(subst::IsRelocatable<T>);

    template <typename>
    friend class Array;

public:
    //-----------------------------------
    // Constructors
    //-----------------------------------
    /*!
    Constructs an empty `Array`.
    */
    PLY_INLINE Array() : items{nullptr}, numItems_{0}, allocated{0} {
    }

    /*!
    Copy constructor. This constructor is always defined because, in C++14, there is no way to
    conditionally disable it if `T` itself is not copy constructible. Instead, a runtime error
    occurs if this constructor is called when `T` is not copy constructible.
    */
    PLY_INLINE Array(const Array& other) {
        ((details::BaseArray&) *this).alloc(other.numItems_, (u32) sizeof(T));
        subst::unsafeConstructArrayFrom(this->items, other.items, other.numItems_);
    }

    /*!
    Move constructor. `other` is reset to an empty `Array`.
    */
    PLY_INLINE Array(Array&& other)
        : items{other.items}, numItems_{other.numItems_}, allocated{other.allocated} {
        other.items = nullptr;
        other.numItems_ = 0;
        other.allocated = 0;
    }

    /*!
    Construct an `Array` from an `ArrayView` of any type from which `T` can be constructed.
    */
    template <typename Other, std::enable_if_t<std::is_constructible<T, Other>::value, int> = 0>
    PLY_INLINE Array(ArrayView<Other> other) {
        ((details::BaseArray&) *this).alloc(other.numItems, (u32) sizeof(T));
        subst::constructArrayFrom(this->items, other.items, this->numItems_);
    }

    /*!
    Constructs an `Array` from a `std::initializer_list<T>`.
    */
    PLY_INLINE Array(std::initializer_list<T> init) {
        ((details::BaseArray&) *this).alloc(safeDemote<u32>(init.size()), (u32) sizeof(T));
        subst::constructArrayFrom(this->items, init.begin(), this->numItems_);
    }

    /*!
    Constructs an `Array` from a `std::initializer_list` of any type from which `T` can be
    constructed.
    */
    template <typename Other, std::enable_if_t<std::is_constructible<T, Other>::value, int> = 0>
    PLY_INLINE Array(std::initializer_list<Other> init) {
        ((details::BaseArray&) *this).alloc(safeDemote<u32>(init.size()), (u32) sizeof(T));
        subst::constructArrayFrom(this->items, init.begin(), this->numItems_);
    }

    PLY_INLINE ~Array() {
        PLY_STATIC_ASSERT(sizeof(Array) ==
                          sizeof(details::BaseArray)); // Sanity check binary compatibility
        subst::destructArray(this->items, this->numItems_);
        PLY_HEAP.free(this->items);
    }

    //-----------------------------------
    // Operators
    //-----------------------------------
    /*!
    Copy assignment operator. This operator is always defined because, in C++14, there is no way to
    conditionally disable it if `T` itself is not copy assignable. Instead, a runtime error occurs
    if this operator is called when `T` is not copy assignable.
    */
    PLY_INLINE void operator=(const Array& other) {
        subst::destructArray(this->items, this->numItems_);
        ((details::BaseArray&) *this).realloc(other.numItems_, (u32) sizeof(T));
        subst::unsafeConstructArrayFrom(this->items, other.items, other.numItems_);
    }

    /*!
    Copy assignment from an `ArrayView` of any type from which `T` can be constructed.
    */
    template <typename Other,
              typename std::enable_if_t<std::is_constructible<T, Other>::value, int> = 0>
    PLY_INLINE void operator=(ArrayView<Other> other) {
        subst::destructArray(this->items, this->numItems_);
        ((details::BaseArray&) *this).realloc(other.numItems, (u32) sizeof(T));
        subst::unsafeConstructArrayFrom(this->items, other.items, other.numItems);
    }

    /*!
    Move assignment operator.
    */
    PLY_INLINE void operator=(Array&& other) {
        this->items = other.items;
        this->numItems_ = other.numItems_;
        this->allocated = other.allocated;
        other.items = nullptr;
        other.numItems_ = 0;
        other.allocated = 0;
    }

    /*!
    Returns the concatenation of two `Array`s. The array items are moved, not copied. Both `Array`
    operands must be rvalue references, and both are reset to empty `Array`s.
    */
    PLY_INLINE Array operator+(Array&& other) && {
        Array result;
        ((details::BaseArray&) result).alloc(this->numItems_ + other.numItems_, (u32) sizeof(T));
        subst::moveConstructArray(result.items, this->items, this->numItems_);
        subst::moveConstructArray(result.items + this->numItems_, other.items, other.numItems_);
        return result;
    }

    //-----------------------------------
    // Lookup
    //-----------------------------------
    /*!
    \beginGroup
    Subscript operator with runtime bounds checking.
    */
    PLY_INLINE T& operator[](u32 index) {
        PLY_ASSERT(index < this->numItems_);
        return this->items[index];
    }
    PLY_INLINE const T& operator[](u32 index) const {
        PLY_ASSERT(index < this->numItems_);
        return this->items[index];
    }
    /*!
    \endGroup
    */

    // Undocumented, might delete later
    PLY_INLINE T* get(u32 index = 0) {
        PLY_ASSERT(index < this->numItems_);
        return this->items + index;
    }
    PLY_INLINE const T* get(u32 index = 0) const {
        PLY_ASSERT(index < this->numItems_);
        return this->items + index;
    }

    /*!
    \beginGroup
    Reverse subscript operator with runtime bound checking. Expects a negative index. `-1` returns
    the last item in the array; `-2` returns the second-last item, etc.
    */
    PLY_INLINE T& back(s32 offset = -1) {
        PLY_ASSERT(offset < 0 && u32(-offset) <= this->numItems_);
        return this->items[this->numItems_ + offset];
    }
    PLY_INLINE const T& back(s32 offset = -1) const {
        PLY_ASSERT(offset < 0 && u32(-offset) <= this->numItems_);
        return this->items[this->numItems_ + offset];
    }
    /*!
    \endGroup
    */

    /*!
    Explicit conversion to `bool`. Returns `true` if the array is not empty. Allows you to use an
    `Array` object inside an `if` condition.

        if (array) {
            ...
        }
    */
    PLY_INLINE explicit operator bool() const {
        return this->numItems_ > 0;
    }

    /*!
    Returns `true` if the array is empty.
    */
    PLY_INLINE bool isEmpty() const {
        return this->numItems_ == 0;
    }

    /*!
    Returns the number of items in the array.
    */
    PLY_INLINE u32 numItems() const {
        return this->numItems_;
    }

    /*!
    Returns the total size, in bytes, of the items in the array. Equivalent to `this->numItems() *
    sizeof(T)`.
    */
    PLY_INLINE u32 sizeBytes() const {
        return this->numItems_ * (u32) sizeof(T);
    }

    /*!
    \beginGroup
    Explicitly create an `ArrayView` into the array. The `ArrayView` items are `const` or
    non-`const` depending on whether the `Array` itself is const.
    */
    PLY_INLINE ArrayView<T> view() { // FIXME: Try using implicit casts instead
        return {this->items, this->numItems_};
    }
    PLY_INLINE ArrayView<const T> view() const {
        return {this->items, this->numItems_};
    }
    /*!
    \endGroup
    */

    /*!
    \beginGroup
    Returns a subview that starts at the offset given by `start`. The optional `numItems` argument
    determines the number of items in the subview. If `numItems` is not specified, the subview
    continues to the end of the view. The subview items are `const` or non-`const` depending on
    whether the `Array` itself is const.
    */
    PLY_INLINE ArrayView<T> subView(u32 start) {
        return view().subView(start);
    }
    PLY_INLINE ArrayView<const T> subView(u32 start) const {
        return view().subView(start);
    }
    PLY_INLINE ArrayView<T> subView(u32 start, u32 numItems_) {
        return view().subView(start, numItems_);
    }
    PLY_INLINE ArrayView<const T> subView(u32 start, u32 numItems_) const {
        return view().subView(start, numItems_);
    }
    /*!
    \endGroup
    */

    //-----------------------------------
    // Modification
    //-----------------------------------
    /*!
    Ensures the underlying memory block is large enough to accomodate the given number of items. If
    `numItems` is greater than the current capacity, the memory block is reallocated. No items are
    constructed or destructed by this function. This function can help avoid repeated memory
    reallocation as the array grows in size.
    */
    PLY_INLINE void reserve(u32 numItems) {
        ((details::BaseArray&) *this).reserve(numItems, (u32) sizeof(T));
    }

    /*!
    Resizes the array to the given number of items. If `numItems` is greater that the current array
    size, new items are constructed. If `numItems` is smaller than the current array size, existing
    items are destructed.
    */
    PLY_NO_INLINE void resize(u32 numItems) {
        if (numItems < this->numItems_) {
            subst::destructArray(this->items + numItems, this->numItems_ - numItems);
        }
        ((details::BaseArray&) *this).reserve(numItems, (u32) sizeof(T));
        if (numItems > this->numItems_) {
            subst::constructArray(this->items + this->numItems_, numItems - this->numItems_);
        }
        this->numItems_ = numItems;
    }

    /*!
    Shrinks the size of the underlying memory block to exactly fit the current number of array
    items.
    */
    PLY_INLINE void truncate() {
        ((details::BaseArray&) *this).truncate((u32) sizeof(T));
    }

    /*!
    \beginGroup
    Appends a single item to the array and returns a reference to it. The arguments are forwarded
    directly to the item's constructor. The returned reference remains valid until the array is
    resized or reallocated.
    */
    PLY_INLINE T& append(T&& item) {
        if (this->numItems_ >= this->allocated) {
            ((details::BaseArray&) *this).reserveIncrement((u32) sizeof(T));
        }
        T* result = new (this->items + this->numItems_) T{std::move(item)};
        this->numItems_++;
        return *result;
    }
    PLY_INLINE T& append(const T& item) {
        if (this->numItems_ >= this->allocated) {
            ((details::BaseArray&) *this).reserveIncrement((u32) sizeof(T));
        }
        T* result = new (this->items + this->numItems_) T{item};
        this->numItems_++;
        return *result;
    }
    template <typename... Args>
    PLY_INLINE T& append(Args&&... args) {
        if (this->numItems_ >= this->allocated) {
            ((details::BaseArray&) *this).reserveIncrement((u32) sizeof(T));
        }
        T* result = new (this->items + this->numItems_) T{std::forward<Args>(args)...};
        this->numItems_++;
        return *result;
    }
    /*!
    \endGroup
    */

    /*!
    Appends multiple items to the array. The new array items are copy constructed from `view`.
    */
    PLY_NO_INLINE void extend(ArrayView<const T> view) {
        ((details::BaseArray&) *this).reserve(this->numItems_ + view.numItems, (u32) sizeof(T));
        subst::constructArrayFrom(this->items + this->numItems_, view.items, view.numItems);
        this->numItems_ += view.numItems;
    }

    /*!
    Appends multiple items to the array. The new array items are move constructed from `view`.
    */
    PLY_NO_INLINE void moveExtend(ArrayView<T> view) {
        ((details::BaseArray&) *this).reserve(this->numItems_ + view.numItems, (u32) sizeof(T));
        subst::moveConstructArray(this->items + this->numItems_, view.items, view.numItems);
        this->numItems_ += view.numItems;
    }

    /*!
    Shrinks the array by `count` items. Equivalent to `resize(numItems() - count)`.
    */
    PLY_INLINE void pop(u32 count = 1) {
        PLY_ASSERT(count <= this->numItems_);
        resize(this->numItems_ - count);
    }

    /*!
    Inserts `count` new items into the array at offset `pos`. The new items are default constructed.
    All existing items after `pos` are shifted upward to accomodate the new items.
    */
    PLY_NO_INLINE T& insert(u32 pos, u32 count = 1) {
        PLY_ASSERT(pos <= this->numItems_);
        ((details::BaseArray&) *this).reserve(this->numItems_ + count, (u32) sizeof(T));
        memmove(this->items + pos + count, this->items + pos,
                (this->numItems_ - pos) * sizeof(T)); // Underlying type is relocatable
        subst::constructArray(this->items + pos, count);
        this->numItems_ += count;
        return this->items[pos];
    }

    /*!
    Destructs `count` array items starting at offset `pos`. Any remaining items starting at index
    `(count + pos)` are shifted downward to replace the erased items. The size of the array then
    shrinks by `count`.
    */
    PLY_NO_INLINE void erase(u32 pos, u32 count = 1) {
        PLY_ASSERT(pos + count <= this->numItems_);
        subst::destructArray(this->items + pos, count);
        memmove(this->items + pos, this->items + pos + count,
                (this->numItems_ - (pos + count)) * sizeof(T)); // Underlying type is relocatable
        this->numItems_ -= count;
    }

    /*!
    \beginGroup
    Destructs `count` array items starting at offset `pos`. The last `count` items in the array are
    moved to the space previously occupied by the erased items. The size of the array then shrinks
    by `count`.

    When the array is large and number of erased items is small, `eraseQuick()` is typically faster
    than `erase()`.
    */
    PLY_INLINE void eraseQuick(u32 pos) {
        PLY_ASSERT(pos < this->numItems_);
        this->items[pos].~T();
        memcpy(this->items + pos, this->items + (this->numItems_ - 1), sizeof(T));
        this->numItems_--;
    }
    PLY_NO_INLINE void eraseQuick(u32 pos, u32 count) {
        PLY_ASSERT(pos + count <= this->numItems_);
        subst::destructArray(this->items + pos, count);
        memmove(this->items + this->numItems_ - count, this->items + pos,
                count * sizeof(T)); // Underlying type is relocatable
        this->numItems_ -= count;
    }
    /*!
    \endGroup
    */

    /*!
    Destructs all items in the array and frees the internal memory block.
    */
    PLY_NO_INLINE void clear() {
        subst::destructArray(this->items, this->numItems_);
        PLY_HEAP.free(this->items);
        this->items = nullptr;
        this->numItems_ = 0;
        this->allocated = 0;
    }

    // Undocumented function; used to move-assign an Array to a TypedArray.
    PLY_INLINE T* release() {
        T* items = this->items;
        this->items = nullptr;
        this->numItems_ = 0;
        this->allocated = 0;
        return items;
    }

    /*!
    \beginGroup
    Required functions to support range-for syntax. Allows you to iterate over all the items in the
    array as follows:

        for (const T& item : array) {
            ...
        }
    */
    PLY_INLINE T* begin() const {
        return this->items;
    }
    PLY_INLINE T* end() const {
        return this->items + this->numItems_;
    }
    /*!
    \endGroup
    */
};

} // namespace ply
