/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/container/Hash.h>
#include <ply-runtime/memory/Heap.h>

namespace ply {

//------------------------------------------------------------------
// details::HashMap
//------------------------------------------------------------------
namespace details {
struct HashMap {
    static constexpr u32 InitialSize = 4;
    static constexpr u32 LinearSearchLimit = 128;
    static constexpr u8 EmptySlot = 0xff;
    // Flags passed to insertOrFind():
    static constexpr u32 AllowFind = 1;
    static constexpr u32 AllowInsert = 2;

    PLY_STATIC_ASSERT(LinearSearchLimit > 0 &&
                      LinearSearchLimit < 256); // Must fit in CellGroup::deltas

    enum class FindResult {
        NotFound,
        Found,
        InsertedNew,
        Overflow,
    };

    // Must be kept binary compatible with HashMap<>::FindInfo
    struct FindInfo {
        // FIXME: Merge FindResult and return these from internal functions
        u32 idx;
        u8* prevLink;
        void* itemSlot;
    };

    struct Callbacks {
        u32 itemSize = 0;
        bool isTriviallyDestructible = false;
        bool requiresContext = false;
        void (*construct)(void* item, const void* key) = nullptr;
        void (*destruct)(void* item) = nullptr;
        void (*moveConstruct)(void* dstItem, void* srcItem) = nullptr;
        void (*moveAssign)(void* dstItem, void* srcItem) = nullptr;
        u32 (*hash)(const void* key) = nullptr;
        bool (*equal)(const void* item, const void* key, const void* context) = nullptr;
        bool (*equalItems)(const void* item0, const void* item1, const void* context) = nullptr;
    };

    PLY_SFINAE_EXPR_1(HasConstruct, &T0::construct);
    PLY_SFINAE_EXPR_1(HasHash, &T0::hash);
    PLY_SFINAE_EXPR_1(HasComparand, &T0::comparand);
    PLY_SFINAE_EXPR_1(HasEqual, &T0::equal);
    PLY_SFINAE_EXPR_1(HasComparandWithContext, T0::comparand(std::declval<typename T0::Item>(),
                                                             std::declval<typename T0::Context>()));

    template <class, class = void>
    struct Context_ {
        using Type = Empty;
    };
    template <class Traits>
    struct Context_<Traits, void_t<typename Traits::Context>> {
        using Type = typename Traits::Context;
    };

    template <class Traits>
    struct CallbackMaker {
        using Key = typename Traits::Key;
        using Item = typename Traits::Item;
        using Context = typename Context_<Traits>::Type;

        // construct
        template <typename U = Traits, std::enable_if_t<HasConstruct<U>, int> = 0>
        static PLY_NO_INLINE void construct(void* item, const void* key) {
            Traits::construct((Item*) item, *(const Key*) key);
        }
        template <
            typename U = Traits,
            std::enable_if_t<!HasConstruct<U> &&
                                 std::is_constructible<typename U::Item, typename U::Key>::value,
                             int> = 0>
        static PLY_NO_INLINE void construct(void* item, const void* key) {
            new (item) Item{*(const Key*) key};
        }
        template <
            typename U = Traits,
            std::enable_if_t<!HasConstruct<U> &&
                                 !std::is_constructible<typename U::Item, typename U::Key>::value,
                             int> = 0>
        static PLY_NO_INLINE void construct(void* item, const void* key) {
            new (item) Item{};
        }

        // destruct
        static PLY_NO_INLINE void destruct(void* item) {
            ((Item*) item)->~Item();
        }

        // moveConstruct
        static PLY_NO_INLINE void moveConstruct(void* dstItem, void* srcItem) {
            new (dstItem) Item{std::move(*(Item*) srcItem)};
        }

        // moveAssign
        static PLY_NO_INLINE void moveAssign(void* dstItem, void* srcItem) {
            *(Item*) dstItem = std::move(*(Item*) srcItem);
        }

        // hash
        template <typename U = Traits, std::enable_if_t<HasHash<U>, int> = 0>
        static PLY_NO_INLINE u32 hash(const void* key) {
            return Traits::hash(*(const Key*) key);
        }
        template <typename U = Traits,
                  std::enable_if_t<!HasHash<U> && std::is_pointer<typename U::Key>::value, int> = 0>
        static PLY_NO_INLINE u32 hash(const void* key) {
            Hasher hasher;
            hasher.appendPtr(*(const Key*) key);
            return hasher.result();
        }
        template <
            typename U = Traits,
            std::enable_if_t<!HasHash<U> && !std::is_pointer<typename U::Key>::value, int> = 0>
        static PLY_NO_INLINE u32 hash(const void* key) {
            Hasher hasher;
            ((const Key*) key)->appendTo(hasher);
            return hasher.result();
        }

        // equalOp (internal function)
        template <typename U = Traits, std::enable_if_t<HasEqual<U>, int> = 0>
        static PLY_INLINE bool equalOp(const Key& a, const Key& b) {
            return Traits::equal(a, b);
        }
        template <typename U = Traits, std::enable_if_t<!HasEqual<U>, int> = 0>
        static PLY_INLINE bool equalOp(const Key& a, const Key& b) {
            return a == b;
        }

        // equal
        template <typename U = Traits, std::enable_if_t<HasComparandWithContext<U>, int> = 0>
        static PLY_NO_INLINE bool equal(const void* item, const void* key, const void* context) {
            return equalOp(Traits::comparand(*(const Item*) item, *(const Context*) context),
                           *(const Key*) key);
        }
        template <typename U = Traits,
                  std::enable_if_t<!HasComparandWithContext<U> && HasComparand<U>, int> = 0>
        static PLY_NO_INLINE bool equal(const void* item, const void* key, const void*) {
            return equalOp(Traits::comparand(*(const Item*) item), *(const Key*) key);
        }
        template <typename U = Traits, std::enable_if_t<!HasComparand<U>, int> = 0>
        static PLY_NO_INLINE bool equal(const void* item, const void* key, const void*) {
            const Key& comp = *(const Item*) item;
            return equalOp(comp, *(const Key*) key);
        }

        // equalItems (Note: This can be deleted if insertMulti() is deleted)
        template <typename U = Traits, std::enable_if_t<HasComparandWithContext<U>, int> = 0>
        static PLY_NO_INLINE bool equalItems(const void* item0, const void* item1,
                                             const void* context) {
            return equalOp(Traits::comparand(*(const Item*) item0, *(const Context*) context),
                           Traits::comparand(*(const Item*) item1, *(const Context*) context));
        }
        template <typename U = Traits,
                  std::enable_if_t<!HasComparandWithContext<U> && HasComparand<U>, int> = 0>
        static PLY_NO_INLINE bool equalItems(const void* item0, const void* item1, const void*) {
            return equalOp(Traits::comparand(*(const Item*) item0),
                           Traits::comparand(*(const Item*) item1));
        }
        template <typename U = Traits, std::enable_if_t<!HasComparand<U>, int> = 0>
        static PLY_NO_INLINE bool equalItems(const void* item0, const void* item1,
                                             const void* context) {
            const Key& comp0 = *(const Item*) item0;
            const Key& comp1 = *(const Item*) item1;
            return equalOp(comp0, comp1);
        }

        static PLY_INLINE const Callbacks* instance() {
            static Callbacks ins = {
                sizeof(Item),
                std::is_trivially_destructible<Item>::value,
                !std::is_same<Context, Empty>::value,
                &construct,
                &destruct,
                &moveConstruct,
                &moveAssign,
                &hash,
                &equal,
                &equalItems,
            };
            return &ins;
        };
    };

    // Note: Must be kept binary compatible with HashMap<>::CellGroup
    struct CellGroup {
        u8 nextDelta[4]; // EmptySlot means the slot is unused
        u8 firstDelta[4];
        u32 hashes[4];
    };

    // Note: Must be kept binary compatible with HashMap<>
    CellGroup* m_cellGroups;
    u32 m_sizeMask;
    u32 m_population;

    PLY_DLL_ENTRY HashMap(const Callbacks* cb, u32 initialSize);
    PLY_DLL_ENTRY HashMap(HashMap&& other);
    PLY_DLL_ENTRY void moveAssign(const Callbacks* cb, HashMap&& other);
    PLY_DLL_ENTRY void clear(const Callbacks* cb);
    static PLY_DLL_ENTRY CellGroup* createTable(const Callbacks* cb, u32 size = InitialSize);
    static PLY_DLL_ENTRY void destroyTable(const Callbacks* cb, CellGroup* cellGroups, u32 size);
    PLY_DLL_ENTRY void migrateToNewTable(const Callbacks* cb);
    PLY_DLL_ENTRY FindResult findNext(FindInfo* info, const Callbacks* cb,
                                      const void* context) const;
    PLY_DLL_ENTRY FindResult insertOrFind(FindInfo* info, const Callbacks* cb, const void* key,
                                          const void* context, u32 flags);
    PLY_DLL_ENTRY void* insertForMigration(u32 itemSize, u32 hash);
    PLY_DLL_ENTRY void erase(FindInfo* info, const Callbacks* cb, u8*& linkToAdjust);

    // Must be kept binary compatible with HashMap<>::Cursor:
    struct Cursor {
        HashMap* m_map;
        FindInfo m_findInfo;
        FindResult m_findResult;

        PLY_DLL_ENTRY void constructFindWithInsert(const Callbacks* cb, HashMap* map,
                                                   const void* key, const void* context, u32 flags);
    };
};
} // namespace details

//------------------------------------------------------------------
// CursorMixin is some template magic that lets us use different return values for
// Cursor::operator->, depending on whether Item is a pointer or not.
//
// CursorMixin *could* be eliminated by eliminating operator-> from Cursor, or forbidding
// operator-> for some types of Item, or by eliminating both operator-> and operator* and just
// exposing an explicit get() function instead.
//------------------------------------------------------------------
template <class Cursor, typename Item, bool = std::is_pointer<Item>::value>
class CursorMixin {
public:
    PLY_INLINE Item* operator->() {
        Cursor* cursor = static_cast<Cursor*>(this);
        PLY_ASSERT(cursor->m_findInfo.itemSlot);
        return cursor->m_findInfo.itemSlot;
    }

    PLY_INLINE const Item* operator->() const {
        const Cursor* cursor = static_cast<const Cursor*>(this);
        PLY_ASSERT(cursor->m_findInfo.itemSlot);
        return cursor->m_findInfo.itemSlot;
    }
};

template <class Cursor, typename Item>
class CursorMixin<Cursor, Item, true> {
public:
    PLY_INLINE Item operator->() const {
        const Cursor* cursor = static_cast<const Cursor*>(this);
        PLY_ASSERT(*cursor->m_findInfo.itemSlot);
        return *cursor->m_findInfo.itemSlot;
    }
};

//------------------------------------------------------------------
// HashMap
//------------------------------------------------------------------
template <class Traits>
class HashMap {
private:
    using Key = typename Traits::Key;
    using Item = typename Traits::Item;
    using Callbacks = details::HashMap::CallbackMaker<Traits>;
    using Context = typename Callbacks::Context;

    // Note: Must be kept binary compatible with details::HashMap::CellGroup
    struct CellGroup {
        u8 nextDelta[4]; // EmptySlot means the slot is unused
        u8 firstDelta[4];
        u32 hashes[4];
        Item items[4];
    };

    // Note: Must be kept binary compatible with details::HashMap
    CellGroup* m_cellGroups;
    u32 m_sizeMask;
    u32 m_population;

    // Must be kept binary-compatible with details::HashMap::FindInfo:
    struct FindInfo {
        u32 idx;
        u8* prevLink;
        Item* itemSlot; // null means not found
    };

public:
    /*!
    Constructs an empty `HashMap`.
    */
    PLY_INLINE HashMap(u32 initialSize = 8) {
        new (this) details::HashMap(Callbacks::instance(), initialSize);
    }

    /*!
    Move constructor. `other` is set to an invalid state. After this call, it's not legal to insert
    or find items in `other` unless it is set back to a valid state using move assignment.
    */
    PLY_INLINE HashMap(HashMap&& other) {
        new (this) details::HashMap{std::move((details::HashMap&) other)};
        other.m_cellGroups = nullptr;
    }

    PLY_INLINE ~HashMap() {
        if (m_cellGroups) {
            details::HashMap::destroyTable(
                Callbacks::instance(), (details::HashMap::CellGroup*) m_cellGroups, m_sizeMask + 1);
        }
    }

    /*!
    Move assignment operator. `other` is set to an invalid state. After this call, it's not legal
    to insert or find items in `other` unless it is set back to a valid state using move assignment.
    */
    PLY_INLINE void operator=(HashMap&& other) {
        reinterpret_cast<details::HashMap*>(this)->moveAssign(
            Callbacks::instance(), std::move(reinterpret_cast<details::HashMap&>(other)));
        other.m_cellGroups = nullptr;
    }

    /*!
    Destructs all `Item`s in the `HashMap` and sets it to an invalid state. After this call, it's
    not legal to insert or find items in the hash map unless it is set back to a valid state using
    move assignment.
    */
    PLY_INLINE void clear() {
        reinterpret_cast<details::HashMap*>(this)->clear(Callbacks::instance());
    }

    //------------------------------------------------------------------
    // Cursor
    //------------------------------------------------------------------
    class Cursor : public CursorMixin<Cursor, Item> {
    private:
        PLY_STATIC_ASSERT(sizeof(CellGroup) ==
                          sizeof(details::HashMap::CellGroup) + sizeof(Item) * 4);

        friend class HashMap;
        template <class, typename, bool>
        friend class CursorMixin;

        // Must be kept binary-compatible with details::HashMap::Cursor:
        HashMap* m_map;
        FindInfo m_findInfo;
        details::HashMap::FindResult m_findResult;

        // Find without insert
        PLY_INLINE Cursor(HashMap* map, const Key& key, const Context* context) : m_map{map} {
            m_findResult = reinterpret_cast<details::HashMap*>(m_map)->insertOrFind(
                (details::HashMap::FindInfo*) &m_findInfo, Callbacks::instance(), &key, context,
                details::HashMap::AllowFind);
        }
        // Find with insert
        PLY_INLINE Cursor(HashMap* map, const Key& key, const Context* context, u32 flags) {
            reinterpret_cast<details::HashMap::Cursor*>(this)->constructFindWithInsert(
                Callbacks::instance(), (details::HashMap*) map, &key, context, flags);
        }

    public:
        PLY_INLINE void operator=(const Cursor& other) {
            m_map = other.m_map;
            m_findInfo = other.m_findInfo;
            m_findResult = other.m_findResult;
        }
        PLY_INLINE bool isValid() const {
            return m_findResult != details::HashMap::FindResult::NotFound;
        }
        PLY_INLINE bool wasFound() const {
            return m_findResult == details::HashMap::FindResult::Found;
        }
        PLY_INLINE void next(const Context& context = {}) {
            m_findResult = reinterpret_cast<const details::HashMap*>(m_map)->findNext(
                (details::HashMap::FindInfo*) &m_findInfo, Callbacks::instance(), &context);
        }
        PLY_INLINE Item& operator*() {
            PLY_ASSERT(m_findInfo.itemSlot);
            return *m_findInfo.itemSlot;
        }
        PLY_INLINE const Item& operator*() const {
            PLY_ASSERT(m_findInfo.itemSlot);
            return *m_findInfo.itemSlot;
        }
        PLY_INLINE void erase() {
            u8* unusedLink = nullptr;
            reinterpret_cast<details::HashMap*>(m_map)->erase(
                (details::HashMap::FindInfo*) &m_findInfo, Callbacks::instance(), unusedLink);
            m_findResult = details::HashMap::FindResult::NotFound;
        }
        PLY_INLINE void eraseAndAdvance(const Context& context = {}) {
            FindInfo infoToErase = m_findInfo;
            m_findResult = reinterpret_cast<const details::HashMap&>(m_map).findNext(
                (details::HashMap::FindInfo*) &m_findInfo, Callbacks::instance(), &context);
            reinterpret_cast<details::HashMap*>(m_map)->erase(
                (details::HashMap::FindInfo*) &infoToErase, Callbacks::instance(),
                m_findInfo.prevLink);
        }
    };

    //------------------------------------------------------------------
    // ConstCursor
    //------------------------------------------------------------------
    class ConstCursor : public CursorMixin<ConstCursor, Item> {
    private:
        PLY_STATIC_ASSERT(sizeof(CellGroup) ==
                          sizeof(details::HashMap::CellGroup) + sizeof(Item) * 4);

        friend class HashMap;
        template <class, typename, bool>
        friend class CursorMixin;

        const HashMap* m_map;
        FindInfo m_findInfo;
        details::HashMap::FindResult m_findResult;

        // Find without insert
        PLY_INLINE ConstCursor(const HashMap* map, const Key& key, const Context* context)
            : m_map{map} {
            m_findResult =
                ((details::HashMap*) m_map)
                    ->insertOrFind((details::HashMap::FindInfo*) &m_findInfo, Callbacks::instance(),
                                   &key, context, details::HashMap::AllowFind);
        }

    public:
        PLY_INLINE void operator=(const ConstCursor& other) {
            m_map = other.m_map;
            m_findInfo = other.m_findInfo;
            m_findResult = other.m_findResult;
        }
        PLY_INLINE bool isValid() const {
            return m_findResult != details::HashMap::FindResult::NotFound;
        }
        PLY_INLINE bool wasFound() const {
            return m_findResult == details::HashMap::FindResult::Found;
        }
        PLY_INLINE void next(const Context& context = {}) {
            m_findResult = reinterpret_cast<const details::HashMap&>(m_map).findNext(
                (details::HashMap::FindInfo*) &m_findInfo, Callbacks::instance(), &context);
        }
        PLY_INLINE Item& operator*() {
            PLY_ASSERT(m_findInfo.itemSlot);
            return *m_findInfo.itemSlot;
        }
        PLY_INLINE const Item& operator*() const {
            PLY_ASSERT(m_findInfo.itemSlot);
            return *m_findInfo.itemSlot;
        }
    };

    /*!
    Returns `true` if the hash map is empty.
    */
    PLY_INLINE bool isEmpty() const {
        return m_population == 0;
    }

    /*!
    Returns the number of items in the hash map.
    */
    PLY_INLINE u32 numItems() const {
        return m_population;
    }

    /*!
    Find `Key` in the hash map. If no matching `Item` exists, a new item is inserted. Call
    `Cursor::wasFound()` on the return value to determine whether the item was found or inserted.
    Note that when a new item is inserted, it might be the caller's responsibility to ensure the
    hash table is left in a consistent state, depending on the behavior of the Traits class's
    `construct` function.
    */
    PLY_INLINE Cursor insertOrFind(const Key& key, const Context* context = nullptr) {
        return {this, key, context, details::HashMap::AllowFind | details::HashMap::AllowInsert};
    }

    // insertMulti is experimental and should not be used. Will likely delete.
    // FIXME: Delete this function and its associated support code, including Cursor::next()
    PLY_INLINE Cursor insertMulti(const Key& key, const Context* context = nullptr) {
        return {this, key, context, details::HashMap::AllowInsert};
    }

    /*!
    \beginGroup
    Attempts to find `Key` in the hash map. Call `Cursor::wasFound()` on the return value to
    determine whether a matching `Item` was found. If the hash map is `const`, a `ConstCursor` is
    returned instead of a `Cursor`. The difference between `Cursor` and `ConstCursor` is that a
    non-const `Cursor` can be used to erase existing items from the map.
    */
    PLY_INLINE Cursor find(const Key& key, const Context* context = nullptr) {
        return {this, key, context};
    }
    PLY_INLINE ConstCursor find(const Key& key, const Context* context = nullptr) const {
        return {this, key, context};
    }
    /*!
    \endGroup
    */

    //------------------------------------------------------------------
    // Iterator
    //------------------------------------------------------------------
    class Iterator {
    private:
        friend class HashMap;
        HashMap& m_map;
        u32 m_idx;

    public:
        Iterator(HashMap& map, u32 idx) : m_map{map}, m_idx{idx} {
        }

        bool isValid() const {
            return m_idx <= m_map.m_sizeMask;
        }

        void next() {
            for (;;) {
                m_idx++;
                PLY_ASSERT(m_idx <= m_map.m_sizeMask + 1);
                if (m_idx > m_map.m_sizeMask)
                    break;
                CellGroup* group = m_map.m_cellGroups + ((m_idx & m_map.m_sizeMask) >> 2);
                if (group->nextDelta[m_idx & 3] != details::HashMap::EmptySlot)
                    break;
            }
        }

        bool operator!=(const Iterator& other) const {
            PLY_ASSERT(&m_map == &other.m_map);
            return m_idx != other.m_idx;
        }

        void operator++() {
            next();
        }

        Item& operator*() const {
            PLY_ASSERT(m_idx <= m_map.m_sizeMask);
            CellGroup* group = m_map.m_cellGroups + ((m_idx & m_map.m_sizeMask) >> 2);
            PLY_ASSERT(group->nextDelta[m_idx & 3] != details::HashMap::EmptySlot);
            return group->items[m_idx & 3];
        }

        Item* operator->() const {
            return &(**this);
        }
    };

    //------------------------------------------------------------------
    // ConstIterator
    //------------------------------------------------------------------
    class ConstIterator {
    private:
        friend class HashMap;
        const HashMap& m_map;
        u32 m_idx;

    public:
        ConstIterator(const HashMap& map, u32 idx) : m_map{map}, m_idx{idx} {
        }

        bool isValid() const {
            return m_idx <= m_map.m_sizeMask;
        }

        void next() {
            for (;;) {
                m_idx++;
                PLY_ASSERT(m_idx <= m_map.m_sizeMask + 1);
                if (m_idx > m_map.m_sizeMask)
                    break;
                CellGroup* group = m_map.m_cellGroups + ((m_idx & m_map.m_sizeMask) >> 2);
                if (group->nextDelta[m_idx & 3] != details::HashMap::EmptySlot)
                    break;
            }
        }

        bool operator!=(const ConstIterator& other) const {
            PLY_ASSERT(&m_map == &other.m_map);
            return m_idx != other.m_idx;
        }

        void operator++() {
            next();
        }

        const Item& operator*() const {
            PLY_ASSERT(m_idx <= m_map.m_sizeMask);
            CellGroup* group = m_map.m_cellGroups + ((m_idx & m_map.m_sizeMask) >> 2);
            PLY_ASSERT(group->nextDelta[m_idx & 3] != details::HashMap::EmptySlot);
            return group->items[m_idx & 3];
        }

        const Item* operator->() const {
            return &(**this);
        }
    };

    /*!
    \beginGroup
    Required functions to support range-for syntax.
    */
    Iterator begin() {
        if (this->m_sizeMask == 0) {
            return {*this, m_sizeMask + 1};
        }
        Iterator iter{*this, (u32) -1};
        iter.next();
        return iter;
    }
    ConstIterator begin() const {
        if (this->m_sizeMask == 0) {
            return {*this, m_sizeMask + 1};
        }
        ConstIterator iter{*this, (u32) -1};
        iter.next();
        return iter;
    }
    Iterator end() {
        return {*this, m_sizeMask + 1};
    }
    ConstIterator end() const {
        return {*this, m_sizeMask + 1};
    }
    /*!
    \endGroup
    */
};

} // namespace ply
