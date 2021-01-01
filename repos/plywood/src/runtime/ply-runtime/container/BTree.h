/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>

#define PLY_BTREE_VALIDATE 0

namespace ply {

template <class Traits_>
class BTree {
public:
    using Traits = Traits_;
    using Item = typename Traits::Item;
    using Index = typename Traits::Index;

private:
    struct Node;

    struct Link {
        Index index; // All descendants are >= this index
        Node* child;
    };

    struct Node {
        Node* parent;
        u16 size;
        bool isLeaf;

        static Node* createInnerNode() {
            Node* node = (Node*) PLY_HEAP.allocAligned(
                sizeof(Node) + sizeof(Link) * Traits::NodeCapacity, Traits::NodeCapacity);
            new (node) Node{nullptr, 0, false};
            return node;
        }

        static Node* createLeaf() {
            Node* node = (Node*) PLY_HEAP.allocAligned(
                sizeof(Node) + sizeof(Item) * Traits::NodeCapacity, Traits::NodeCapacity);
            new (node) Node{nullptr, 0, true};
            return node;
        }

        Link* getLinks() {
            PLY_ASSERT(!isLeaf);
            return (Link*) (this + 1);
        }

        Item* getItems() {
            PLY_ASSERT(isLeaf);
            return (Item*) (this + 1);
        }

        Node* getChild(const Index& index) {
            PLY_ASSERT(!isLeaf);
            PLY_ASSERT(size > 0);
            Link* links = getLinks();
            u32 lo = 0;
            u32 hi = size - 1;
            while (lo < hi) {
                u32 mid = (lo + hi + 1) / 2;
                if (Traits::less(index, links[mid].index))
                    hi = mid - 1;
                else
                    lo = mid;
            }
            return links[lo].child;
        }

        u32 findLink(const Node* child) {
            PLY_ASSERT(!isLeaf);
            u32 s = size;
            Link* links = getLinks();
            for (u32 i = 0; i < s; i++) {
                if (links[i].child == child)
                    return i;
            }
            PLY_ASSERT(0); // Shouldn't get here
            return 0;
        }

        Node* getRightSibling() {
            PLY_ASSERT(isLeaf);
            Node* node = this;
            for (;;) {
                Node* p = node->parent;
                if (!p)
                    return nullptr;
                u32 pos = p->findLink(node);
                if (pos < u32(p->size) - 1) {
                    node = p->getLinks()[pos + 1].child;
                    break;
                }
                node = p;
            }
            while (!node->isLeaf) {
                node = node->getLinks()[0].child;
            }
            return node;
        }

        Node* getLeftSibling() {
            PLY_ASSERT(isLeaf);
            Node* node = this;
            for (;;) {
                Node* p = node->parent;
                if (!p)
                    return nullptr;
                u32 pos = p->findLink(node);
                if (pos > 0) {
                    node = p->getLinks()[pos - 1].child;
                    break;
                }
                node = p;
            }
            while (!node->isLeaf) {
                node = node->getLinks()[node->size - 1].child;
            }
            return node;
        }

        Index getFirstIndex() {
            PLY_ASSERT(size > 0);
            if (isLeaf) {
                return Traits::getIndex(getItems()[0]);
            } else {
                return getLinks()[0].index;
            }
        }

        void insertLink(Node* child, u32 pos) {
            PLY_ASSERT(!isLeaf);
            PLY_ASSERT(size < Traits::NodeCapacity);
            PLY_ASSERT(pos <= size);
            Link* links = getLinks();
            u32 s = size;
            new (links + s) Link;
            for (u32 i = s; i > pos; i--) {
                links[i] = std::move(links[i - 1]);
            }
            links[pos].index = child->getFirstIndex();
            links[pos].child = child;
            child->parent = this;
            size++;
        }

        void deleteLink(u32 pos) {
            PLY_ASSERT(!isLeaf);
            PLY_ASSERT(pos < size);
            u32 limit = size - 1;
            Link* links = getLinks();
            for (u32 i = pos; i < limit; i++) {
                links[i] = std::move(links[i + 1]);
            }
            links[limit].~Link();
            size--;
            // FIXME: Don't bother updating the index if the Index type doesn't reference
            // items. We only really need this eg. when Index is a StringView into a String
            // owned by an item, as in SemaEntity::nameToChild.
            if (pos == 0 && limit > 0) {
                this->updateIndexInParent(links[0].index);
            }
        }

        u32 findGreaterOrEqualPos(const Index& index) {
            PLY_ASSERT(isLeaf);
            Item* items = getItems();
            u32 lo = 0;
            u32 hi = size;
            while (lo < hi) {
                u32 mid = (lo + hi) / 2;
                const Index& otherIndex = Traits::getIndex(items[mid]);
                if (!Traits::less(otherIndex, index))
                    hi = mid;
                else
                    lo = mid + 1;
            }
            return lo;
        }

        u32 findInsertPos(const Index& index) {
            // This is different from findGreaterOrEqualPos because, in the case of
            // CookJobSchedule, we want inserts with the same timestamp to appear *after* the
            // existing item, so that it acts as a queue.
            PLY_ASSERT(isLeaf);
            Item* items = getItems();
            u32 lo = 0;
            u32 hi = size;
            while (lo < hi) {
                u32 mid = (lo + hi) / 2;
                const Index& otherIndex = Traits::getIndex(items[mid]);
                if (Traits::less(index, otherIndex))
                    hi = mid;
                else
                    lo = mid + 1;
            }
            return lo;
        }

        Item& insertItemAt(u32 pos) {
            PLY_ASSERT(isLeaf);
            PLY_ASSERT(size < Traits::NodeCapacity);
            PLY_ASSERT(pos <= size);
            Item* items = getItems();
            u32 s = size;
            new (items + s) Item();
            for (u32 i = s; i > pos; i--) {
                items[i] = std::move(items[i - 1]);
            }
            size++;
            return items[pos];
        }

        void updateIndexInParent(const Index& index) {
            if (this->parent) {
                u32 i = this->parent->findLink(this);
                this->parent->getLinks()[i].index = index;
                if (i == 0) {
                    this->parent->updateIndexInParent(index);
                }
            }
        }

        void deleteItem(u32 pos) {
            PLY_ASSERT(isLeaf);
            PLY_ASSERT(pos < size);
            u32 limit = size - 1;
            Item* items = getItems();
            for (u32 i = pos; i < limit; i++) {
                items[i] = std::move(items[i + 1]);
            }
            items[limit].~Item();
            size--;
            // FIXME: Don't bother updating the index if the Index type doesn't reference
            // items. We only really need this eg. when Index is a StringView into a String
            // owned by an item, as in SemaEntity::nameToChild.
            if (pos == 0 && limit > 0) {
                this->updateIndexInParent(Traits::getIndex(items[0]));
            }
        }

        Node* splitLeaf() {
            PLY_ASSERT(isLeaf);
            PLY_ASSERT(size >= 2);
            u32 s = size;
            u32 mid = s / 2;
            Node* rightSibling = createLeaf();
            Item* leftItems = getItems();
            Item* rightItems = rightSibling->getItems();
            for (u32 i = 0; i < s - mid; i++) {
                Item* src = leftItems + (mid + i);
                Item* dst = rightItems + i;
                new (dst) Item(std::move(*src));
                Traits::onItemMoved(*dst, rightSibling);
                src->~Item();
            }
            size = mid;
            rightSibling->size = (s - mid);
            return rightSibling;
        }

        Node* splitInnerNode() {
            PLY_ASSERT(!isLeaf);
            PLY_ASSERT(size >= 2);
            u32 s = size;
            u32 mid = s / 2;
            Node* rightSibling = createInnerNode();
            Link* leftLinks = getLinks();
            Link* rightLinks = rightSibling->getLinks();
            for (u32 i = 0; i < s - mid; i++) {
                Link* src = leftLinks + (mid + i);
                Link* dst = rightLinks + i;
                new (dst) Link(std::move(*src));
                dst->child->parent = rightSibling;
                src->~Link();
            }
            size = mid;
            rightSibling->size = (s - mid);
            return rightSibling;
        }

        Node* merge(Node* rightSibling) {
            PLY_ASSERT(isLeaf == rightSibling->isLeaf);
            PLY_ASSERT(parent == rightSibling->parent);
            PLY_ASSERT(size + rightSibling->size <= Traits::NodeCapacity);
            u32 ls = size;
            u32 rs = rightSibling->size;
            if (isLeaf) {
                Item* leftItems = getItems();
                Item* rightItems = rightSibling->getItems();
                for (u32 i = 0; i < rs; i++) {
                    Item* src = rightItems + i;
                    Item* dst = leftItems + (ls + i);
                    new (dst) Item(std::move(*src));
                    Traits::onItemMoved(*dst, this);
                    src->~Item();
                }
            } else {
                Link* leftLinks = getLinks();
                Link* rightLinks = rightSibling->getLinks();
                for (u32 i = 0; i < rs; i++) {
                    Link* src = rightLinks + i;
                    Link* dst = leftLinks + (ls + i);
                    new (dst) Link(std::move(*src));
                    dst->child->parent = this;
                    src->~Link();
                }
            }
            size += rs;
            PLY_HEAP.freeAligned(rightSibling);
            return this;
        }

        void destroyRecursively() {
            u32 s = size;
            if (isLeaf) {
                Item* items = getItems();
                for (u32 i = 0; i < s; i++) {
                    items[i].~Item();
                }
            } else {
                Link* links = getLinks();
                for (u32 i = 0; i < s; i++) {
                    links[i].child->destroyRecursively();
                    links[i].~Link();
                }
            }
            PLY_HEAP.freeAligned(this);
        }

#if PLY_BTREE_VALIDATE
        void validate(u32 parentLinkIndex) {
            PLY_ASSERT(size > 0);
            if (isLeaf) {
                // Items must be in increasing order
                Index limit = parent ? parent->getLinks()[parentLinkIndex].index
                                     : Traits::getIndex(getItems()[0]);
                for (u32 i = 0; i < size; i++) {
                    const Index& index = Traits::getIndex(getItems()[i]);
                    PLY_ASSERT(limit <= index);
                    limit = index;
                }
            } else {
                // Links must be in increasing order
                Link* links = getLinks();
                Index limit = parent ? parent->getLinks()[parentLinkIndex].index : links[0].index;
                for (u32 i = 0; i < size; i++) {
                    PLY_ASSERT(links[i].child->parent == this);
                    const Index& index = links[i].index;
                    PLY_ASSERT(limit <= index);
                    limit = index;
                    links[i].child->validate(i);
                }
            }
        }
#endif
    };

    Node* m_root = nullptr;

    void insertRightChild(Node* node, Node* leftChild, Node* rightChild) {
        if (node) {
            PLY_ASSERT(!node->isLeaf);
            u32 rightPos = node->findLink(leftChild) + 1;
            if (node->size >= Traits::NodeCapacity) {
                Node* rightSibling = node->splitInnerNode();
                insertRightChild(node->parent, node, rightSibling);
                if (rightPos > node->size) {
                    rightPos -= node->size;
                    node = rightSibling;
                    PLY_ASSERT(rightPos <= node->size);
                }
            }
            node->insertLink(rightChild, rightPos);
        } else {
            // Create new root node for both children
            PLY_ASSERT(m_root == leftChild);
            Node* node = Node::createInnerNode();
            node->size = 2;
            Link* links = node->getLinks();
            new (links + 0) Link{leftChild->getFirstIndex(), leftChild};
            leftChild->parent = node;
            new (links + 1) Link{rightChild->getFirstIndex(), rightChild};
            rightChild->parent = node;
            m_root = node;
        }
    }

    Node* split(Node* node) {
        PLY_ASSERT(node->isLeaf);
        Node* rightSibling = node->splitLeaf();
        insertRightChild(node->parent, node, rightSibling);
        return rightSibling;
    }

    void tryCompact(Node* node) {
        // Note: This BTree is not really self-balancing because we never "steal" from
        // siblings. We only ever merge two siblings into one. For example, it's possible for
        // the left-most or right-most child of any node to contain only a single item or link
        // (if it couldn't be merged with its sibling). If we wanted self-balancing behavior,
        // we would steal from a sibling any time a node became less than half-full.
        Node* parent = node->parent;
        if (parent) {
            Link* links = parent->getLinks();
            u32 pos = parent->findLink(node);
            if (node->size == 0) {
                PLY_HEAP.freeAligned(node);
                parent->deleteLink(pos);
                tryCompact(parent);
            } else {
                if (pos > 0) {
                    Node* leftSibling = links[pos - 1].child;
                    PLY_ASSERT(node->isLeaf == leftSibling->isLeaf);
                    if (node->size + leftSibling->size <= Traits::NodeCapacity) {
                        node = leftSibling->merge(node);
                        parent->deleteLink(pos);
                        tryCompact(parent);
                        return;
                    }
                    // Fall through to other test
                }
                if (pos < u32(parent->size) - 1) {
                    Node* rightSibling = links[pos + 1].child;
                    PLY_ASSERT(node->isLeaf == rightSibling->isLeaf);
                    if (node->size + rightSibling->size <= Traits::NodeCapacity) {
                        node->merge(rightSibling);
                        parent->deleteLink(pos + 1);
                        tryCompact(parent);
                    }
                }
            }
        } else {
            PLY_ASSERT(node == m_root);
            if (node->size == 0) {
                clear();
            } else if (node->size == 1 && !node->isLeaf) {
                m_root = node->getLinks()[0].child;
                m_root->parent = nullptr;
                node->deleteLink(0);
                PLY_HEAP.freeAligned(node);
            }
        }
    }

public:
    class Iterator {
    private:
        static constexpr uptr PosMask = uptr(Traits::NodeCapacity - 1);
        friend class BTree;
        uptr leafPos;

        Node* getLeaf() const {
            Node* leaf = (Node*) (leafPos & ~PosMask);
            PLY_ASSERT(leaf->isLeaf);
            return leaf;
        }

        u32 getPos() const {
            return leafPos & PosMask;
        }

    public:
        Iterator() : leafPos{0} {
        }

        Iterator(Node* leaf, u32 pos) {
            PLY_ASSERT(isAlignedPowerOf2((uptr) leaf, Traits::NodeCapacity));
            PLY_ASSERT(pos < Traits::NodeCapacity);
            leafPos = uptr(leaf) | pos;
        }

        // Note: the "end" of the tree is considered valid
        bool isValid() const {
            return (leafPos != 0);
        }

        Item& getItem() const {
            PLY_ASSERT(isValid());
            return getLeaf()->getItems()[getPos()];
        }

        void next() {
            // Why don't we assert here?
            if (leafPos != 0) {
                Node* leaf = getLeaf();
                if (getPos() + 1 < leaf->size) {
                    leafPos++;
                    return;
                }
                leafPos = (uptr) leaf->getRightSibling(); // pos is 0
                PLY_ASSERT(isAlignedPowerOf2(leafPos, Traits::NodeCapacity));
            }
        }

        void prev() {
            PLY_ASSERT(isValid()); // possibly points beyond end, but that's OK
            if (getPos() > 0) {
                leafPos--;
            } else {
                Node* left = getLeaf()->getLeftSibling();
                PLY_ASSERT(left->size > 0);
                leafPos = (uptr) left + (left->size - 1);
            }
        }

        bool operator!=(const Iterator& other) {
            return leafPos != other.leafPos;
        }

        void operator++() {
            next();
        }

        Item& operator*() const {
            return getItem();
        }

        Item* operator->() const {
            return &getItem();
        }
    };

private:
    void updateLinkIndex(const Iterator& iter) {
        Node* node = iter.getLeaf();
        if (node->size > 0 && iter.getPos() == 0) {
            Node* parent = node->parent;
            if (parent) {
                PLY_ASSERT(!parent->isLeaf);
                const Index& newIndex = Traits::getIndex(node->getItems()[0]);
                u32 linkPos;
                do {
                    linkPos = parent->findLink(node);
                    //                    PLY_ASSERT(linkPos == 0 || newIndex >=
                    //                    parent->getLinks()[linkPos - 1].index);
                    PLY_ASSERT(linkPos == 0 ||
                               !Traits::less(newIndex, parent->getLinks()[linkPos - 1].index));
                    //                    PLY_ASSERT(linkPos == parent->size - 1 || newIndex <=
                    //                    parent->getLinks()[linkPos + 1].index);
                    PLY_ASSERT(linkPos == (u32) parent->size - 1 ||
                               !Traits::less(parent->getLinks()[linkPos + 1].index, newIndex));
                    Index& linkIndex = parent->getLinks()[linkPos].index;
                    if (newIndex == linkIndex)
                        return;
                    linkIndex = newIndex;
                    node = parent;
                    parent = node->parent;
                } while (parent && linkPos == 0);
            }
        }
    }

public:
    BTree() {
        PLY_ASSERT(isPowerOf2(Traits::NodeCapacity));
    }

    BTree(BTree&& other) : m_root{other.m_root} {
        other.m_root = nullptr;
    }

    ~BTree() {
        if (m_root)
            m_root->destroyRecursively();
    }

    bool isEmpty() const {
        return (m_root == nullptr);
    }

    void clear() {
        if (m_root) {
            m_root->destroyRecursively();
            m_root = nullptr;
        }
    }

    Iterator findLastLessThan(const Index& index) const {
        if (!m_root)
            return {nullptr, 0};
        Node* node = m_root;
        while (!node->isLeaf) {
            node = node->getChild(index);
        }
        u32 pos = node->findGreaterOrEqualPos(index);
        if (pos > 0) {
            return {node, pos - 1};
        } else {
            node = node->getLeftSibling();
            return {node, node ? node->size - 1 : 0u};
        }
    }

    Iterator findFirstGreaterOrEqualTo(const Index& index) const {
        if (!m_root)
            return {nullptr, 0};
        Node* node = m_root;
        while (!node->isLeaf) {
            node = node->getChild(index);
        }
        u32 pos = node->findGreaterOrEqualPos(index);
        if (pos >= node->size)
            return {node->getRightSibling(), 0};
        return {node, pos};
    }

    Iterator prepareInsert(const Index& index) {
        if (!m_root)
            m_root = Node::createLeaf();
        Node* node = m_root;
        while (!node->isLeaf) {
            node = node->getChild(index);
        }
        u32 pos = node->findInsertPos(index);
        if (node->size >= Traits::NodeCapacity) {
            Node* rightSibling = split(node);
            if (pos > node->size) {
                pos -= node->size;
                node = rightSibling;
            }
        }
        return Iterator{node, pos};
    }

    void insert(const Item& item) {
        Iterator iter = prepareInsert(Traits::getIndex(item));
        Item& dstItem = iter.getLeaf()->insertItemAt(iter.getPos());
        dstItem = item;
        Traits::onItemMoved(dstItem, iter.getLeaf());
        updateLinkIndex(iter);
    }

    void insert(Item&& item) {
        Iterator iter = prepareInsert(Traits::getIndex(item));
        Item& dstItem = iter.getLeaf()->insertItemAt(iter.getPos());
        dstItem = std::move(item);
        Traits::onItemMoved(dstItem, iter.getLeaf());
        updateLinkIndex(iter);
    }

    // locate() is used when items maintain pointers back to their own Nodes, such as in
    // CookJobSchedule. "context" is just a pointer to the Node with the type erased. (In
    // CookJobSchedule, this back-pointer is maintained in Job::btContext, and it gets updated
    // each time Traits::onItemMoved() is called.) (This avoids having to search for the Job in
    // the CookJobSchedule from scratch each time. Maybe this strategy is a little bit overkill
    // and it would be OK to just search for the Job from scratch each time... I'll leave it
    // as-is for now.)
    template <typename MatchItem>
    Iterator locate(void* context, const MatchItem& item) {
        Node* inLeaf = (Node*) context;
        PLY_ASSERT(inLeaf->isLeaf);
        u32 s = inLeaf->size;
        Item* items = inLeaf->getItems();
        for (u32 i = 0; i < s; i++) {
            if (Traits::equal(items[i], item))
                return Iterator{inLeaf, i};
        }
        PLY_ASSERT(0); // Shouldn't get here
        return Iterator{nullptr, 0};
    }

    void checkParents(Iterator& iter) {
        PLY_ASSERT(!isEmpty());
        Node* node = iter.getLeaf();
        PLY_ASSERT(node->size > iter.getPos());
        Index index = Traits::getIndex(node->getItems()[iter.getPos()]);
        while (node->parent) {
            u32 pos = node->parent->findLink(node);
            const Index& linkIndex = node->parent->getLinks()[pos].index;
            PLY_ASSERT(linkIndex <= index);
            node = node->parent;
            index = linkIndex;
        }
    }

    Item remove(Iterator& iter) {
        PLY_ASSERT(!isEmpty());
        Node* node = iter.getLeaf();
        PLY_ASSERT(node->size > iter.getPos());
        Item* items = node->getItems();
        Item result = std::move(items[iter.getPos()]);
        node->deleteItem(iter.getPos());
        updateLinkIndex(iter);
        tryCompact(node);
        return result;
    }

    // FIXME: Support const iteration
    Iterator begin() {
        Node* node = m_root;
        if (node) {
            while (!node->isLeaf) {
                PLY_ASSERT(node->size > 0);
                node = node->getLinks()[0].child;
            }
            return Iterator{node, 0};
        } else {
            return Iterator{nullptr, 0};
        }
    }

    Iterator end() {
        return Iterator{nullptr, 0};
    }

    Item& front() {
        PLY_ASSERT(!isEmpty());
        return begin().getItem();
    }

    Item popFront() {
        PLY_ASSERT(!isEmpty());
        Iterator iter = begin();
        return remove(iter);
    }

#if PLY_BTREE_VALIDATE
    void validate() {
        if (m_root) {
            PLY_ASSERT(m_root->parent == nullptr);
            m_root->validate(0);
            // Ensure all items are in increasing order
            Index limit = m_root->isLeaf ? Traits::getIndex(m_root->getItems()[0])
                                         : m_root->getLinks()[0].index;
            for (const Item& item : *this) {
                const Index& index = Traits::getIndex(item);
                PLY_ASSERT(limit <= index);
                limit = index;
            }
        }
    }
#endif
};

} // namespace ply
