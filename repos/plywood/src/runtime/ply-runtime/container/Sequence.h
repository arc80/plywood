/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/container/Array.h>
#include <ply-runtime/container/details/SequenceImpl.h>

namespace ply {

template <typename T>
class SequenceReader;

template <typename T>
class SequenceIterator;

//------------------------------------------------------------------------------------------------
/*!
A `Sequence` object contains a sequence of items. You can add items to the end and remove items from
the beginning or the end. These operations let you use a `Sequence` as a stack or a queue.
`Sequence` is built on top of `BlockList` and offers an alternative to `Array` that can grow to an
arbitrary number of items without ever reallocating memory. Once an item is added to a `Sequence`,
the item's address never changes. The items are destructed and freed when the `Sequence` is
destroyed.
*/
template <typename T>
class Sequence {
private:
    details::SequenceImpl impl;
    friend class SequenceReader<T>;

public:
    /*!
    \category Constructors
    Constructs an empty `Sequence`.

        Sequence<String> list;
    */
    PLY_INLINE Sequence() = default;

    /*!
    Move constructor. `other` is left in an unusable state and cannot be modified after this
    operation.

        Sequence<String> arr = std::move(other);
    */
    PLY_INLINE Sequence(Sequence&& other) : impl{std::move(other.impl)} {
    }

    /*!
    Destructor. Destructs all items and frees the memory associated with the `Sequence`.
    */
    PLY_INLINE ~Sequence() {
        details::destructSequence<T>(this->impl.head);
    }

    /*!
    \category Assignment
    Move assignment operator. `other` is left in an unusable state and cannot be modified after this
    operation.

        arr = std::move(other);
    */
    PLY_INLINE void operator=(Sequence&& other) {
        details::destructSequence<T>(this->impl.head);
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
        if ((this->impl.tail->blockSize - this->impl.tail->numBytesUsed) < sizeof(T)) {
            this->impl.beginWriteInternal(sizeof(T));
        }
        return ArrayView<T>::from(this->impl.tail->viewUnusedBytes());
    }

    /*!
    Returns a pointer to a single uninitialized item. The caller is meant to construct this item,
    then call `endWrite`.

        String* item = list.beginWriteNoConstruct();
        new (item) String;
        list.endWrite();
    */
    PLY_INLINE T* beginWriteNoConstruct() {
        if (this->impl.tail->viewUnusedBytes().numBytes < sizeof(T)) {
            return (T*) this->impl.beginWriteInternal(sizeof(T));
        } else {
            return (T*) this->impl.tail->unused();
        }
    }

    /*!
    Must be called after `beginWriteViewNoConstruct` or `beginWriteNoConstruct` after constructing
    some number of items. Advances the write position and makes the newly constructed items
    available for read.
    */
    PLY_INLINE void endWrite(u32 numItems = 1) {
        PLY_ASSERT(sizeof(T) * numItems <=
                   this->impl.tail->blockSize - this->impl.tail->numBytesUsed);
        this->impl.tail->numBytesUsed += sizeof(T) * numItems;
    }

    /*!
    \beginGroup
    Appends a single item to the list and returns a reference to it. The arguments are forwarded
    directly to the item's constructor. The returned reference remains valid until the `Sequence`
    is destroyed or `moveToArray` is called.

        Sequence<String> list;
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
    Destructs all items in the list and frees any associated memory. The `Sequence` is left in an
    empty state.
    */
    PLY_INLINE void clear() {
        *this = Sequence{};
    }

    /*!
    \category Conversion
    Converts the list of items to an `Array`. When the number of items fits in a single chunk, the
    chunk is truncated and directly adopted by the `Array`, avoiding unnecessary memory reallocation
    or copying. The `Sequence` is left in an unusable state and cannot be modified after this
    operation.

        Sequence<String> list;
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
    SequenceIterator<T> begin() const;
    SequenceIterator<T> end() const;
    /*!
    \endGroup
    */
};

//-----------------------------------------------------------
// SequenceReader
//-----------------------------------------------------------
template <typename T>
class SequenceReader {
private:
    details::SequenceReaderImpl impl;
    friend class SequenceIterator<T>;

    PLY_INLINE SequenceReader() = default;

public:
    PLY_INLINE SequenceReader(const Sequence<T>& src) : impl{src.impl} {
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
    // SequenceIterator
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
// SequenceIterator
//-----------------------------------------------------------
template <typename T>
class SequenceIterator {
private:
    SequenceReader<T> reader;
    friend class Sequence<T>;
    friend class Sequence<typename std::remove_const<T>::type>;

    PLY_INLINE SequenceIterator(const Sequence<T>& src) : reader{src} {
    }
    PLY_INLINE SequenceIterator() {
    }

public:
    PLY_INLINE bool operator!=(const SequenceIterator&) const {
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
PLY_INLINE SequenceIterator<T> Sequence<T>::begin() const {
    return *this;
}

template <typename T>
PLY_INLINE SequenceIterator<T> Sequence<T>::end() const {
    return {};
}

} // namespace ply
