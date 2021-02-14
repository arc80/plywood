/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/container/ArrayView.h>
#include <ply-runtime/container/details/BaseArray.h>
#include <ply-runtime/memory/Heap.h>
#include <ply-runtime/string/StringMixin.h>

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

`Array` is implicitly convertible to `ArrayView` and can be passed directly to any function that
expects an `ArrayView`.
*/
template <typename T>
class Array {
private:
    T* items;
    u32 numItems_;
    u32 allocated;

    // T cannot be const
    PLY_STATIC_ASSERT(!std::is_const<T>::value);

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
    /*!
    \category Constructors
    Constructs an empty `Array`.

        Array<u32> arr;
    */
    PLY_INLINE Array() : items{nullptr}, numItems_{0}, allocated{0} {
    }

    /*!
    Copy constructor. This constructor is always defined because, in C++14, there is no way to
    conditionally disable it if `T` itself is not copy constructible. Instead, a runtime error
    occurs if this constructor is called when `T` is not copy constructible.

        Array<u32> arr = other;
    */
    PLY_INLINE Array(const Array& other) {
        ((details::BaseArray&) *this).alloc(other.numItems_, (u32) sizeof(T));
        subst::unsafeConstructArrayFrom(this->items, other.items, other.numItems_);
    }

    /*!
    Move constructor. `other` is reset to an empty `Array`.

        Array<u32> arr = std::move(other);
    */
    PLY_INLINE Array(Array&& other)
        : items{other.items}, numItems_{other.numItems_}, allocated{other.allocated} {
        other.items = nullptr;
        other.numItems_ = 0;
        other.allocated = 0;
    }

    /*!
    Constructs an `Array` from a braced initializer list.

        Array<u32> arr = {4, 5, 6};
    */
    PLY_NO_INLINE Array(InitList<T> init) {
        u32 initSize = safeDemote<u32>(init.size());
        ((details::BaseArray&) *this).alloc(initSize, (u32) sizeof(T));
        subst::constructArrayFrom(this->items, init.begin(), initSize);
    }

    /*!
    Construct an `Array` from any array-like object (ie. `Array`, `ArrayView`, `FixedArray`, etc.)
    of any type from which `T` can be constructed. The other object must have a member function
    named `view()` that returns an `ArrayView`.

        FixedArray<u16, 3> fixed = {4, 5, 6};
        Array<u32> arr = fixed;

    Move semantics are used when the source object owns its items and can be moved from. In the
    following example, the right-hand side is a temporary `Array<String>` that can be moved from,
    so each `HybridString` in the result is constructed by moving from each `String` in the
    temporary object.

        Array<HybridString> arr = Array<String>{"hello", "there"};
    */
    template <typename Other, typename U = details::ArrayViewType<Other>>
    PLY_INLINE Array(Other&& other) {
        ((details::BaseArray&) *this).alloc(ArrayView<U>{other}.numItems, (u32) sizeof(T));
        details::moveOrCopyConstruct(this->items, std::forward<Other>(other));
    }

    /*!
    Destructor. Destructs all items and frees the memory associated with the `Array`.
    */
    PLY_INLINE ~Array() {
        PLY_STATIC_ASSERT(sizeof(Array) ==
                          sizeof(details::BaseArray)); // Sanity check binary compatibility
        subst::destructArray(this->items, this->numItems_);
        PLY_HEAP.free(this->items);
    }

    /*!
    \category Assignment Operators
    Copy assignment operator. This operator is always defined because, in C++14, there is no way to
    conditionally disable it if `T` itself is not copy assignable. Instead, a runtime error occurs
    if this operator is called when `T` is not copy assignable.

        arr = other;
    */
    PLY_INLINE void operator=(const Array& other) {
        subst::destructArray(this->items, this->numItems_);
        ((details::BaseArray&) *this).realloc(other.numItems_, (u32) sizeof(T));
        subst::unsafeConstructArrayFrom(this->items, other.items, other.numItems_);
    }

    /*!
    Move assignment operator. `other` is reset to an empty `Array`.

        arr = std::move(other);
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
    Assignment from a braced initializer list.

        Array<u32> arr;
        arr = {4, 5, 6};
    */
    PLY_INLINE void operator=(InitList<T> init) {
        subst::destructArray(this->items, this->numItems_);
        u32 initSize = safeDemote<u32>(init.size());
        ((details::BaseArray&) *this).realloc(initSize, (u32) sizeof(T));
        subst::unsafeConstructArrayFrom(this->items, init.begin(), initSize);
    }

    /*!
    Construct an `Array` from any array-like object (ie. `Array`, `ArrayView`, `FixedArray`, etc.)
    of any type from which `T` can be constructed. The other object must have a member function
    named `view()` that returns an `ArrayView`.

        Array<u32> arr;
        FixedArray<u16, 3> fixed = {4, 5, 6};
        arr = fixed;

    Move semantics are used when the source object owns its items and can be moved from. In the
    following example, the right-hand side is a temporary `Array<String>` that can be moved from,
    so each `HybridString` in the result is constructed by moving from each `String` in the
    temporary object.

        Array<HybridString> arr;
        arr = Array<String>{"hello", "there"}; // uses move semantics
    */
    template <typename Other, typename U = details::ArrayViewType<Other>>
    PLY_INLINE void operator=(Other&& other) {
        subst::destructArray(this->items, this->numItems_);
        ((details::BaseArray&) *this).realloc(ArrayView<U>{other}.numItems, (u32) sizeof(T));
        details::moveOrCopyConstruct(this->items, std::forward<Other>(other));
    }

    /*!
    \category Element Access
    \beginGroup
    Subscript operator with runtime bounds checking.

        Array<u32> arr = {4, 5, 6};
        arr[1];  // 5
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

        Array<u32> arr = {4, 5, 6};
        arr.back();  // 6
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
    \beginGroup
    Required functions to support range-for syntax. Allows you to iterate over all the items in the
    array as follows:

        for (const T& item : arr) {
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

    /*!
    \category Capacity
    Explicit conversion to `bool`. Returns `true` if the array is not empty. Allows you to use an
    `Array` object inside an `if` condition.

        if (arr) {
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
    \category Modifiers
    Destructs all items in the array and frees the internal memory block.
    */
    PLY_NO_INLINE void clear() {
        subst::destructArray(this->items, this->numItems_);
        PLY_HEAP.free(this->items);
        this->items = nullptr;
        this->numItems_ = 0;
        this->allocated = 0;
    }

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
    \beginGroup
    Appends multiple items to the array. The argument can be a braced initializer list or an
    array-like object (ie. `Array`, `ArrayView`, `FixedArray`, etc.) of any type from which `T` can
    be constructed.

        Array<String> arr;
        arr.extend({"hello", "there"});
        arr.extend(ArrayView<const StringView>{"my", "friend"});

    Move semantics are used when the source object owns its items and can be moved from. In the
    following example, the argument to `extend` is a temporary `Array<String>` that can be moved
    from, so each `HybridString` in the result is appended by moving from each `String` in the
    temporary object.

        Array<HybridString> arr;
        arr.extend(Array<String>{"hello", "there"}); // uses move semantics
    */
    PLY_INLINE void extend(InitList<T> init) {
        u32 initSize = safeDemote<u32>(init.size());
        ((details::BaseArray&) *this).reserve(this->numItems_ + initSize, (u32) sizeof(T));
        subst::constructArrayFrom(this->items + this->numItems_, init.begin(), initSize);
        this->numItems_ += initSize;
    }
    template <typename Other, typename U = details::ArrayViewType<Other>>
    PLY_INLINE void extend(Other&& other) {
        u32 numOtherItems = ArrayView<U>{other}.numItems;
        ((details::BaseArray&) *this).reserve(this->numItems_ + numOtherItems, (u32) sizeof(T));
        details::moveOrCopyConstruct(this->items + this->numItems_, std::forward<Other>(other));
        this->numItems_ += numOtherItems;
    }
    /*!
    \endGroup
    */

    /*!
    Appends multiple items to the `Array` from an `ArrayView`. The new array items are move
    constructed from the items in `other`.
    */
    PLY_NO_INLINE void moveExtend(ArrayView<T> other) {
        ((details::BaseArray&) *this).reserve(this->numItems_ + other.numItems, (u32) sizeof(T));
        subst::moveConstructArray(this->items + this->numItems_, other.items, other.numItems);
        this->numItems_ += other.numItems;
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
        memmove(static_cast<void*>(this->items + pos + count),
                static_cast<const void*>(this->items + pos),
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
        memmove(static_cast<void*>(this->items + pos),
                static_cast<const void*>(this->items + pos + count),
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
        memcpy(static_cast<void*>(this->items + pos),
               static_cast<const void*>(this->items + (this->numItems_ - 1)), sizeof(T));
        this->numItems_--;
    }
    PLY_NO_INLINE void eraseQuick(u32 pos, u32 count) {
        PLY_ASSERT(pos + count <= this->numItems_);
        subst::destructArray(this->items + pos, count);
        memmove(static_cast<void*>(this->items + pos),
                static_cast<const void*>(this->items + this->numItems_ - count),
                count * sizeof(T)); // Underlying type is relocatable
        this->numItems_ -= count;
    }
    /*!
    \endGroup
    */

    // Undocumented function; used to move-assign an Array to a TypedArray.
    PLY_INLINE T* release() {
        T* items = this->items;
        this->items = nullptr;
        this->numItems_ = 0;
        this->allocated = 0;
        return items;
    }

    template <typename Arr0, typename Arr1, typename, typename>
    friend PLY_INLINE auto operator+(Arr0&& a, Arr1&& b);

    /*!
    \category Convert to View
    \beginGroup
    Explicitly create an `ArrayView` into the array.
    */
    PLY_INLINE ArrayView<T> view() {
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
    Implicit conversion to `ArrayView`. Makes is possible to pass `Array` to any function that
    expects an `ArrayView`.
    */
    PLY_INLINE operator ArrayView<T>() {
        return {this->items, this->numItems_};
    }
    PLY_INLINE operator ArrayView<const T>() const {
        return {this->items, this->numItems_};
    }
    /*!
    \endGroup
    */
    /*!
    \beginGroup
    Explicitly creeate a `StringView` or `MutableStringView` into the array.
    */
    PLY_INLINE StringView stringView() const {
        return {(const char*) this->items, safeDemote<u32>(this->numItems_ * sizeof(T))};
    }
    PLY_INLINE MutableStringView mutableStringView() const {
        return {(char*) this->items, safeDemote<u32>(this->numItems_ * sizeof(T))};
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
};

namespace details {
template <typename T>
struct InitListType<Array<T>> {
    using Type = ArrayView<const T>;
};

template <typename T>
struct ArrayTraits<Array<T>> {
    using ItemType = T;
    static constexpr bool IsOwner = true;
};
} // namespace details

/*!
\addToClass Array
\category Modifiers
Returns the concatenation of two array-like objects `a` and `b`. The arguments can be instances of
`Array`, `ArrayView`, `FixedArray` or any class that has a member function named `view()` returning
an `ArrayView`. The returned `Array` has the same item type as `a`, and `b`'s items must be
convertible to this type.

Move semantics are used when either operand owns its items and can be moved from.
*/
template <typename Arr0, typename Arr1, typename T0 = details::ArrayViewType<Arr0>,
          typename T1 = details::ArrayViewType<Arr1>>
PLY_INLINE auto operator+(Arr0&& a, Arr1&& b) {
    u32 numItemsA = ArrayView<T0>{a}.numItems;
    u32 numItemsB = ArrayView<T1>{b}.numItems;

    Array<std::remove_const_t<T0>> result;
    ((details::BaseArray&) result).alloc(numItemsA + numItemsB, (u32) sizeof(T0));
    details::moveOrCopyConstruct(result.items, std::forward<Arr0>(a));
    details::moveOrCopyConstruct(result.items + numItemsA, std::forward<Arr1>(b));
    return result;
}

} // namespace ply
