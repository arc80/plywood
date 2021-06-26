/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/container/Array.h>
#include <ply-runtime/container/details/TypedListImpl.h>

namespace ply {

template <typename T>
class TypedListReader;

template <typename T>
class TypedListIterator;

//------------------------------------------------------------------------------------------------
/*!
A `TypedList` object contains a list of typed items. Internally, a `TypedList` maintains a linked
list of chunks, with the items stored contiguously in memory within each chunk. `TypedList` offers
an alternative to `Array` that can grow to an arbitrary number of items without ever reallocating
memory. Once an item is added to a `TypedList`, the item's address never changes. The items are
destructed and freed when the `TypedList` is destroyed.
*/
template <typename T>
class TypedList {
private:
    details::TypedListImpl impl;
    friend class TypedListReader<T>;

public:
    /*!
    \category Constructors
    Constructs an empty `TypedList`.

        TypedList<String> list;
    */
    PLY_INLINE TypedList() = default;

    /*!
    Move constructor. `other` is reset to an empty `TypedList`.

        TypedList<String> arr = std::move(other);
    */
    PLY_INLINE TypedList(TypedList&& other) : impl{std::move(other.impl)} {
    }

    /*!
    Destructor. Destructs all items and frees the memory associated with the `TypedList`.
    */
    PLY_INLINE ~TypedList() {
        details::destructTypedList<T>(this->impl.headChunk);
    }

    /*!
    \category Assignment
    Move assignment operator. `other` is reset to an empty `TypedList`.

        arr = std::move(other);
    */
    PLY_INLINE void operator=(TypedList&& other) {
        details::destructTypedList<T>(this->impl.headChunk);
        this->impl = std::move(other.impl);
    }

    /*!
    \category Capacity
    Returns `true` if the list is empty.
    */
    PLY_INLINE bool isEmpty() const {
        return this->impl.isEmpty();
    }

    /*!
    \category Modification
    Returns an `ArrayView` of allocated but uninitialized items. The returned `ArrayView` always
    contains at least one item. The caller is meant to construct some number of items, then call
    `endWrite`.

        ArrayView<String> view = list.beginWriteViewNoConstruct();
        for (u32 i = 0; i < view.numItems; i++) {
            new (&view[i]) String{String::from(i)};
        }
        list.endWrite(view.numItems);
    */
    PLY_INLINE ArrayView<T> beginWriteViewNoConstruct() {
        if (this->impl.tailChunk->numRemainingBytes() < sizeof(T)) {
            this->impl.beginWriteInternal(sizeof(T));
        }
        return ArrayView<T>::from(this->impl.tailChunk->viewRemainingBytes());
    }

    /*!
    Returns a pointer to a single uninitialized item. The caller is meant to construct this item,
    then call `endWrite`.

        String* item = list.beginWriteNoConstruct();
        new (item) String;
        list.endWrite();
    */
    PLY_INLINE T* beginWriteNoConstruct() {
        if (this->impl.tailChunk->numRemainingBytes() < sizeof(T)) {
            return (T*) this->impl.beginWriteInternal(sizeof(T));
        } else {
            return (T*) this->impl.tailChunk->curByte();
        }
    }

    /*!
    Must be called after `beginWriteViewNoConstruct` or `beginWriteNoConstruct` after constructing
    some number of items. Advances the write position and makes the newly constructed items
    available for read.
    */
    PLY_INLINE void endWrite(u32 numItems = 1) {
        PLY_ASSERT(sizeof(T) * numItems <= this->impl.tailChunk->numRemainingBytes());
        this->impl.tailChunk->writePos += sizeof(T) * numItems;
    }

    /*!
    \beginGroup
    Appends a single item to the list and returns a reference to it. The arguments are forwarded
    directly to the item's constructor. The returned reference remains valid until the `TypedList`
    is destroyed or `moveToArray` is called.

        TypedList<String> list;
        list.append("Hello");
    */
    PLY_INLINE T& append(const T& item) {
        T* result = beginWriteNoConstruct();
        new (result) T{item};
        endWrite();
        return *result;
    }

    PLY_INLINE T& append(T&& item) {
        T* result = beginWriteNoConstruct();
        new (result) T{std::move(item)};
        endWrite();
        return *result;
    }

    template <typename... Args>
    PLY_INLINE T& append(Args&&... args) {
        T* result = beginWriteNoConstruct();
        new (result) T{std::forward<Args>(args)...};
        endWrite();
        return *result;
    }

    /*!
    \endGroup
    */
    /*!
    Destructs all items in the list and frees the internal chunk list.
    */
    PLY_INLINE void clear() {
        *this = TypedList{};
    }

    /*!
    \category Conversion
    Converts the list of items to an `Array` and resets the `TypedList` to an empty state. When the
    number of items fits in a single chunk, the chunk is truncated and directly adopted by the
    `Array`, avoiding unnecessary memory reallocation or copying.

        TypedList<String> list;
        list.append("Hello");
        Array<String> arr = list.moveToArray();
    */
    PLY_INLINE Array<T> moveToArray() {
        String str = this->impl.moveToString();
        u32 numItems = str.numBytes / sizeof(T); // Divide by constant is fast
        return Array<T>::adopt((T*) str.release(), numItems);
    }

    /*!
    \beginGroup
    \category Iteration
    Required functions to support range-for syntax. Allows you to iterate over all the items in the
    list.

        for (const T& item : list) {
            ...
        }
    */
    TypedListIterator<T> begin() const;
    TypedListIterator<T> end() const;
    /*!
    \endGroup
    */
};

//-----------------------------------------------------------
// TypedListReader
//-----------------------------------------------------------
template <typename T>
class TypedListReader {
private:
    details::TypedListReaderImpl impl;
    friend class TypedListIterator<T>;

    PLY_INLINE TypedListReader() = default;

public:
    PLY_INLINE TypedListReader(const TypedList<T>& src) : impl{src.impl} {
    }

    // FIXME: Move ctor/assignment

    PLY_INLINE ArrayView<const T> beginReadView() {
        u32 numBytes = this->impl.tryMakeBytesAvailable(sizeof(T));
        u32 numItems = numBytes / safeDemote<u32>(sizeof(T)); // Divide by constant is fast
        PLY_ASSERT(numItems * sizeof(T) == numBytes);
        return {(T*) this->impl.curByte, numItems};
    }

    // beginRead() should only be called when you know !atEOF()
    // It's safe to call beginRead() repeatedly before endRead(), as might happen when using
    // TypedListIterator
    PLY_INLINE const T& beginRead() {
        u32 numBytes = this->impl.tryMakeBytesAvailable(sizeof(T));
        PLY_ASSERT(numBytes >= sizeof(T));
        return *(T*) this->impl.curByte;
    }

    PLY_INLINE void endRead(u32 numItems) {
        PLY_ASSERT(this->impl.numBytesAvailable() >= sizeof(T) * numItems);
        // FIXME: Eliminate safeDemote everywhere in this file, also make it constexpr
        this->impl.curByte += safeDemote<u32>(sizeof(T)) * numItems;
    }

    // Unlike InStream::atEOF, this returns true before the caller attempts to read the last item
    PLY_INLINE bool atEOF() const {
        return this->impl.atEOF();
    }

    // Memory remains valid until the next call to read/beginRead
    PLY_INLINE const T& read() {
        const T& item = this->beginRead();
        this->endRead(1);
        return item;
    }
};

//-----------------------------------------------------------
// TypedListIterator
//-----------------------------------------------------------
template <typename T>
class TypedListIterator {
private:
    TypedListReader<T> reader;
    friend class TypedList<T>;
    friend class TypedList<typename std::remove_const<T>::type>;

    PLY_INLINE TypedListIterator(const TypedList<T>& src) : reader{src} {
    }
    PLY_INLINE TypedListIterator() {
    }

public:
    PLY_INLINE bool operator!=(const TypedListIterator&) const {
        return !this->reader.atEOF();
    }

    PLY_INLINE void operator++() {
        this->reader.endRead(1);
    }

    PLY_INLINE const T& operator*() {
        return this->reader.beginRead();
    }

    PLY_INLINE const T& operator->() {
        return this->reader.beginRead();
    }
};

//-----------------------------------------------------------
// Member functions
//-----------------------------------------------------------
template <typename T>
PLY_INLINE TypedListIterator<T> TypedList<T>::begin() const {
    return *this;
}

template <typename T>
PLY_INLINE TypedListIterator<T> TypedList<T>::end() const {
    return {};
}

} // namespace ply
