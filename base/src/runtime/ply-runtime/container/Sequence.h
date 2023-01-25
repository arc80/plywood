/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/container/BlockList.h>

namespace ply {

namespace impl {
PLY_DLL_ENTRY void destructSequence(Reference<BlockList::Footer>* headRef,
                                    void (*destructViewAs)(StringView));
PLY_DLL_ENTRY void beginWriteInternal(BlockList::Footer** tail, u32 numBytes);
PLY_DLL_ENTRY void popTail(BlockList::Footer** tail, u32 numBytes,
                           void (*destructViewAs)(StringView));
PLY_DLL_ENTRY void truncate(BlockList::Footer** tail, const BlockList::WeakRef& to);
PLY_DLL_ENTRY u32 getTotalNumBytes(BlockList::Footer* head);
PLY_DLL_ENTRY char* read(BlockList::WeakRef* weakRef, u32 itemSize);
} // namespace impl

template <typename T>
class Sequence;

//-----------------------------------------------------------
// WeakSequenceRef
//-----------------------------------------------------------
template <typename T>
class WeakSequenceRef {
private:
    BlockList::WeakRef impl;
    template <typename>
    friend class Sequence;

    PLY_INLINE WeakSequenceRef(const BlockList::WeakRef& impl) : impl{impl} {
    }

public:
    PLY_INLINE WeakSequenceRef() = default;
    PLY_INLINE ArrayView<T> beginRead() {
        sptr numBytesAvailable = this->impl.block->unused() - this->impl.byte;
        if (numBytesAvailable == 0) {
            numBytesAvailable = BlockList::jumpToNextBlock(&impl);
        } else {
            // numBytesAvailable should always be a multiple of sizeof(T).
            PLY_ASSERT(numBytesAvailable >= sizeof(T));
        }
        return ArrayView<T>::from(StringView{this->impl.byte, numBytesAvailable});
    }
    PLY_INLINE void endRead(u32 numItems) {
        PLY_ASSERT(this->impl.block->unused() - this->impl.byte >= sizeof(T) * numItems);
        this->impl.byte += sizeof(T) * numItems;
    }
    PLY_INLINE WeakSequenceRef normalized() const {
        return this->impl.normalized();
    }

    // Range for support.
    PLY_INLINE T& operator*() const {
        // It is illegal to call operator* at the end of the sequence.
        PLY_ASSERT(this->impl.block->unused() - this->impl.byte >= sizeof(T));
        return *(T*) this->impl.byte;
    }
    PLY_INLINE T* operator->() const {
        // It is illegal to call operator-> at the end of the sequence.
        PLY_ASSERT(this->impl.block->unused() - this->impl.byte >= sizeof(T));
        return (T*) this->impl.byte;
    }
    PLY_INLINE void operator++() {
        sptr numBytesAvailable = this->impl.block->unused() - this->impl.byte;
        // It is illegal to call operator++ at the end of the sequence.
        PLY_ASSERT(numBytesAvailable >= sizeof(T));
        this->impl.byte += sizeof(T);
        numBytesAvailable -= sizeof(T);
        if (numBytesAvailable == 0) {
            numBytesAvailable = BlockList::jumpToNextBlock(&impl);
            // We might now be at the end of the sequence.
        } else {
            // numBytesAvailable should always be a multiple of sizeof(T).
            PLY_ASSERT(numBytesAvailable >= sizeof(T));
        }
    }
    PLY_INLINE void operator--() {
        sptr numBytesPreceding = this->impl.byte - this->impl.block->start();
        if (numBytesPreceding == 0) {
            numBytesPreceding = BlockList::jumpToPrevBlock(&impl);
        }
        // It is illegal to call operator-- at the start of the sequence.
        PLY_ASSERT(numBytesPreceding >= sizeof(T));
        this->impl.byte -= sizeof(T);
    }
    PLY_INLINE bool operator!=(const WeakSequenceRef& other) const {
        return this->impl.byte != other.impl.byte;
    }

    // The reference returned here remains valid as long as item continues to exist in the
    // underlying sequence.
    PLY_INLINE T& read() {
        return *(T*) impl::read(&impl, sizeof(T));
    }
    PLY_INLINE void* byte() const {
        return this->impl.byte;
    }
};

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
    Reference<BlockList::Footer> headBlock;
    BlockList::Footer* tailBlock = nullptr;

public:
    /*!
    \category Constructors
    Constructs an empty `Sequence`.

        Sequence<String> list;
    */
    PLY_INLINE Sequence() : headBlock{BlockList::createBlock()}, tailBlock{headBlock} {
    }

    /*!
    Move constructor. `other` is left in an unusable state and cannot be modified after this
    operation.

        Sequence<String> arr = std::move(other);
    */
    PLY_INLINE Sequence(Sequence&& other) : headBlock{std::move(other.headBlock)}, tailBlock{other.tailBlock} {
    }

    /*!
    Destructor. Destructs all items and frees the memory associated with the `Sequence`.
    */
    PLY_INLINE ~Sequence() {
        impl::destructSequence(&headBlock, subst::destructViewAs<T>);
    }

    /*!
    \category Assignment
    Move assignment operator. `other` is left in an unusable state and cannot be modified after this
    operation.

        arr = std::move(other);
    */
    PLY_INLINE void operator=(Sequence&& other) {
        impl::destructSequence(&headBlock, subst::destructViewAs<T>);
        new (this) Sequence{std::move(other)};
    }

    /*!
    \category Element Access
    Returns the last item in the sequence. The sequence must not be empty.
    */
    PLY_INLINE T& head() {
        // It is illegal to call head() on an empty sequence.
        PLY_ASSERT(this->headBlock->viewUsedBytes().numBytes >= sizeof(T));
        return *(T*) this->headBlock->start();
    }
    PLY_INLINE T& tail() {
        // It is illegal to call tail() on an empty sequence.
        PLY_ASSERT(this->tailBlock->viewUsedBytes().numBytes >= sizeof(T));
        return ((T*) this->tailBlock->unused())[-1];
    }

    /*!
    \category Capacity
    Returns `true` if the sequence is empty.
    */
    PLY_INLINE bool isEmpty() const {
        // Only an empty sequence can have an empty head block.
        return this->headBlock->viewUsedBytes().isEmpty();
    }
    PLY_INLINE u32 numItems() const {
        // Fast division by integer constant.
        return impl::getTotalNumBytes(this->headBlock) / sizeof(T);
    }

    /*!
    \category Modification
    Returns an `ArrayView` of allocated but uninitialized items. The returned `ArrayView` always
    contains at least one item. The caller is meant to construct some number of items, then call
    `endWrite`.

        ArrayView<String> view = list.beginWriteViewNoConstruct();
        for (u32 i = 0; i < view.numItems; i++) {
            new (&view[i]) String{to_string(i)};
        }
        list.endWrite(view.numItems);
    */
    PLY_INLINE ArrayView<T> beginWriteViewNoConstruct() {
        if (this->tailBlock->viewUnusedBytes().numBytes < sizeof(T)) {
            impl::beginWriteInternal(&this->tailBlock, sizeof(T));
        }
        return ArrayView<T>::from(this->tailBlock->viewUnusedBytes());
    }

    /*!
    Returns a pointer to a single uninitialized item. The caller is meant to construct this item,
    then call `endWrite`.

        String* item = list.beginWriteNoConstruct();
        new (item) String;
        list.endWrite();
    */
    PLY_INLINE T* beginWriteNoConstruct() {
        if (this->tailBlock->viewUnusedBytes().numBytes < sizeof(T)) {
            impl::beginWriteInternal(&this->tailBlock, sizeof(T));
        }
        return (T*) this->tailBlock->unused();
    }

    /*!
    Must be called after `beginWriteViewNoConstruct` or `beginWriteNoConstruct` after constructing
    some number of items. Advances the write position and makes the newly constructed items
    available for read.
    */
    PLY_INLINE void endWrite(u32 numItems = 1) {
        PLY_ASSERT(sizeof(T) * numItems <= this->tailBlock->viewUnusedBytes().numBytes);
        this->tailBlock->numBytesUsed += sizeof(T) * numItems;
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
    Deletes the last `numItems` items from the sequence.
    */
    PLY_INLINE void popTail(u32 numItems = 1) {
        impl::popTail(&this->tailBlock, numItems * (u32) sizeof(T), subst::destructViewAs<T>);
    }
    PLY_INLINE void truncate(const WeakSequenceRef<T>& to) {
        impl::truncate(&this->tailBlock, to.impl);
    }

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
        char* startByte = this->headBlock->start();
        String str = BlockList::toString({std::move(this->headBlock), startByte});
        u32 numItems = str.numBytes / sizeof(T); // Divide by constant is fast
        return Array<T>::adopt((T*) str.release(), numItems);
    }

    /*!
    \beginGroup
    \category Iteration
    Required functions to support range-for syntax. Allows you to iterate over all the items in
    the list.

        for (const T& item : list) {
            ...
        }
    */
    WeakSequenceRef<T> begin() {
        return BlockList::WeakRef{this->headBlock, this->headBlock->start()};
    }
    WeakSequenceRef<T> end() {
        return BlockList::WeakRef{this->tailBlock, this->tailBlock->unused()};
    }
    WeakSequenceRef<const T> begin() const {
        return BlockList::WeakRef{this->headBlock, this->headBlock->start()};
    }
    WeakSequenceRef<const T> end() const {
        return BlockList::WeakRef{this->tailBlock, this->tailBlock->unused()};
    }
    /*!
    \endGroup
    */
};

} // namespace ply
