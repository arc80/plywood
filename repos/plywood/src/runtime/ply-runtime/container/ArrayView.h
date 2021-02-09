/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/container/Subst.h>

namespace ply {

struct StringView;
struct MutableStringView;

//------------------------------------------------------------------------------------------------
/*!
An `ArrayView` references a range of typed items in memory. It consists of a pointer `items` and an
integer `numItems`. An `ArrayView` does not own the memory it points to, and no heap memory is freed
when the `ArrayView` is destroyed.

When an `ArrayView` is `const`, the items its references are immutable. When an `ArrayView` is not
`const`, the items its references are mutable only if the item type `T` is not `const`.
`ArrayView<T>` is implicitly convertible to `ArrayView<const T>` and can be passed as an argument to
any function that expects `ArrayView<const T>`.
 */
template <typename T_>
struct ArrayView {
    typedef T_ T;

    /*!
    A pointer to the first item in the `ArrayView`.
    */
    T* items = nullptr;

    /*!
    The number of items in the `ArrayView`.
    */
    u32 numItems = 0;

    /*!
    Constructs an empty `ArrayView`.
    */
    PLY_INLINE ArrayView() = default;

    /*!
    Constructs an `ArrayView` from the given pointer and number of items.
    */
    PLY_INLINE ArrayView(T* items, u32 numItems) : items{items}, numItems{numItems} {
    }

    /*!
    Constructs an `ArrayView` from a braced initializer list. Lets you pass an initializer list
    to any function expecting an `ArrayView`, as in the following:

        void foo(ArrayView<u32>);
        void test() {
            foo({1, 2, 3, 4, 5});
        }

    Use this carefully; the underlying array only exists for the lifetime of the statement calling
    this constructor.
    */
    PLY_INLINE ArrayView(std::initializer_list<T> init)
        : items{init.begin()}, numItems{safeDemote<u32>(init.size())} {
        PLY_STATIC_ASSERT(std::is_const<T>::value);
        // Guaranteed by the standard:
        PLY_ASSERT((uptr) init.end() - (uptr) init.begin() == sizeof(T) * init.size());
    }

    /*!
    Conversion operator. Makes `ArrayView<T>` implicitly convertible to `ArrayView<const T>`.
    */
    PLY_INLINE operator ArrayView<const T>() const {
        return {this->items, this->numItems};
    }
    PLY_INLINE ArrayView<T> view() const {
        return *this;
    }

    /*!
    \beginGroup
    Explicitly convert the `ArrayView` to a `StringView` or `MutableStringView`.
    */
    PLY_INLINE StringView stringView() const;
    PLY_INLINE MutableStringView mutableStringView();
    /*!
    \endGroup
    */

    /*!
    \beginGroup
    Subscript operator with runtime bounds checking.
    */
    PLY_INLINE T& operator[](u32 index) {
        PLY_ASSERT(index < numItems);
        return items[index];
    }
    PLY_INLINE const T& operator[](u32 index) const {
        PLY_ASSERT(index < numItems);
        return items[index];
    }
    /*!
    \endGroup
    */

    /*!
    \beginGroup
    Reverse subscript operator with runtime bound checking. Expects a negative index. `-1` returns
    the last item in the view; `-2` returns the second-last item, etc.
    */
    PLY_INLINE T& back(s32 offset = -1) {
        PLY_ASSERT(u32(numItems + offset) < numItems);
        return items[numItems + offset];
    }
    PLY_INLINE const T& back(s32 offset = -1) const {
        PLY_ASSERT(u32(numItems + offset) < numItems);
        return items[numItems + offset];
    }
    /*!
    \endGroup
    */

    /*!
    Increases `items` and decreases `numItems` by `ofs`. Equivalent to:

        *this = this->subView(ofs);
    */
    PLY_INLINE void offsetHead(u32 ofs) {
        PLY_ASSERT(ofs <= numItems);
        items += ofs;
        numItems -= ofs;
    }

    /*!
    Adds `ofs`, which must be less than or equal to zero, to `numItems`. Equivalent to:

        *this = this->subView(0, this->numItems + ofs);
    */
    PLY_INLINE void offsetBack(s32 ofs) {
        PLY_ASSERT((u32) -ofs <= numItems);
        numItems += ofs;
    }

    /*!
    Explicit conversion to `bool`. Returns `true` if `numItems` is greater than 0. Allows you to use
    an `ArrayView` object inside an `if` condition.

        if (view) {
            ...
        }
    */
    PLY_INLINE explicit operator bool() const {
        return this->numItems > 0;
    }

    /*!
    Returns `true` if `numItems` is 0.
    */
    PLY_INLINE bool isEmpty() const {
        return numItems == 0;
    }

    /*!
    Returns the total size, in bytes, of the items in the view. Equivalent to `this->numItems *
    sizeof(T)`.
    */
    PLY_INLINE u32 sizeBytes() const {
        return numItems * u32(sizeof(T));
    }

    /*!
    \beginGroup
    Returns a subview that starts at the offset given by `start`. The optional `numItems` argument
    determines the number of items in the subview. If `numItems` is not specified, the subview
    continues to the end of the view.
    */
    PLY_INLINE ArrayView subView(u32 start) const {
        PLY_ASSERT(start <= numItems);
        return {items + start, numItems - start};
    }
    PLY_INLINE ArrayView subView(u32 start, u32 numItems) const {
        PLY_ASSERT(start <= this->numItems); // FIXME: Support different end parameters
        PLY_ASSERT(start + numItems <= this->numItems);
        return {items + start, numItems};
    }
    /*!
    \endGroup
    */

    /*!
    Returns a subview with the last `numItems` items of the view omitted.
    */
    PLY_INLINE ArrayView shortenedBy(u32 numItems) const {
        PLY_ASSERT(numItems <= this->numItems);
        return {this->items, this->numItems - numItems};
    }

    /*!
    \beginGroup
    Required functions to support range-for syntax. Allows you to iterate over all the items in the
    view as follows:

        for (const T& item : view) {
            ...
        }
    */
    PLY_INLINE T* begin() const {
        return items;
    }
    PLY_INLINE T* end() const {
        return items + numItems;
    }
    /*!
    \endGroup
    */
};

namespace details {

// If ArrayViewType<Arr> is well-formed, that means Arr can be converted to an ArrayView by calling
// its view() member function, and ArrayViewType<Arr> resolves to the ArrayView's template argument.
template <typename Arr>
using ArrayViewType = std::remove_pointer_t<decltype(std::declval<Arr>().view().items)>;

template <typename Arr>
struct CanMoveFromArrayLike : std::false_type {};

// If the second argument is an rvalue reference, Arr will be deduced as non-reference.
// Otherwise, Arr will be deduced as a reference, and IsArrayOwner<Arr> will always be false.
// Therefore, the moving version of this function will only be called if the second argument is an
// rvalue reference to an array-like class that owns its elements, which is appropriate.
template <typename T, typename Arr, std::enable_if_t<CanMoveFromArrayLike<Arr>::value, int> = 0>
PLY_INLINE void moveOrCopyConstruct(T* dst, Arr&& src) {
    ArrayView<ArrayViewType<Arr>> srcView = src.view();
    subst::moveConstructArray(dst, srcView.items, srcView.numItems);
}
template <typename T, typename Arr, std::enable_if_t<!CanMoveFromArrayLike<Arr>::value, int> = 0>
PLY_INLINE void moveOrCopyConstruct(T* dst, Arr&& src) {
    ArrayView<ArrayViewType<Arr>> srcView = src.view();
    subst::constructArrayFrom(dst, srcView.items, srcView.numItems);
}

} // namespace details

/*!
\addToClass ArrayView
\beginGroup
Compares two array-like objects. `a` and `b` can have different item types. Returns `true` if and
only if `a` and `b` have the same number of items and `a[i] == b[i]` evaluates to `true` at every
index `i`.
*/
template <typename T0, typename T1>
PLY_NO_INLINE bool operator==(ArrayView<T0> a, ArrayView<T1> b) {
    if (a.numItems != b.numItems)
        return false;
    for (u32 i = 0; i < a.numItems; i++) {
        if (!(a[i] == b[i]))
            return false;
    }
    return true;
}
template <typename Arr0, typename Arr1, typename = details::ArrayViewType<Arr0>,
          typename = details::ArrayViewType<Arr1>>
PLY_INLINE bool operator==(const Arr0& a, const Arr1& b) {
    return a.view() == b.view();
}
/*!
\endGroup
*/

#define PLY_ALLOC_STACK_ARRAY(T, count) \
    ArrayView<T> { \
        (T*) alloca(sizeof(T) * (count)), (count) \
    }

} // namespace ply
