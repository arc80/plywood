/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
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
        bool is_leaf;

        static Node* create_inner_node() {
            Node* node = (Node*) Heap.alloc_aligned(
                sizeof(Node) + sizeof(Link) * Traits::NodeCapacity,
                Traits::NodeCapacity);
            new (node) Node{nullptr, 0, false};
            return node;
        }

        static Node* create_leaf() {
            Node* node = (Node*) Heap.alloc_aligned(
                sizeof(Node) + sizeof(Item) * Traits::NodeCapacity,
                Traits::NodeCapacity);
            new (node) Node{nullptr, 0, true};
            return node;
        }

        Link* get_links() {
            PLY_ASSERT(!is_leaf);
            return (Link*) (this + 1);
        }

        Item* get_items() {
            PLY_ASSERT(is_leaf);
            return (Item*) (this + 1);
        }

        Node* get_last_child_less_than(const Index& index) {
            PLY_ASSERT(!is_leaf);
            PLY_ASSERT(size > 0);
            Link* links = get_links();
            u32 lo = 0;
            u32 hi = size - 1;
            while (lo < hi) {
                u32 mid = (lo + hi + 1) / 2;
                if (Traits::less(links[mid].index, index))
                    lo = mid;
                else
                    hi = mid - 1;
            }
            return links[lo].child;
        }

        u32 find_link(Node* child) {
            PLY_ASSERT(!is_leaf);
            u32 s = size;
            Link* links = get_links();
            for (u32 i = 0; i < s; i++) {
                if (links[i].child == child)
                    return i;
            }
            PLY_ASSERT(0); // Shouldn't get here
            return 0;
        }

        Node* get_right_sibling() {
            PLY_ASSERT(is_leaf);
            Node* node = this;
            for (;;) {
                Node* p = node->parent;
                if (!p)
                    return nullptr;
                u32 pos = p->find_link(node);
                if (pos < u32(p->size) - 1) {
                    node = p->get_links()[pos + 1].child;
                    break;
                }
                node = p;
            }
            while (!node->is_leaf) {
                node = node->get_links()[0].child;
            }
            return node;
        }

        Node* get_left_sibling() {
            PLY_ASSERT(is_leaf);
            Node* node = this;
            for (;;) {
                Node* p = node->parent;
                if (!p)
                    return nullptr;
                u32 pos = p->find_link(node);
                if (pos > 0) {
                    node = p->get_links()[pos - 1].child;
                    break;
                }
                node = p;
            }
            while (!node->is_leaf) {
                node = node->get_links()[node->size - 1].child;
            }
            return node;
        }

        Index get_first_index() {
            PLY_ASSERT(size > 0);
            if (is_leaf) {
                return Traits::get_index(get_items()[0]);
            } else {
                return get_links()[0].index;
            }
        }

        void insert_link(Node* child, u32 pos) {
            PLY_ASSERT(!is_leaf);
            PLY_ASSERT(size < Traits::NodeCapacity);
            PLY_ASSERT(pos <= size);
            Link* links = get_links();
            u32 s = size;
            new (links + s) Link;
            for (u32 i = s; i > pos; i--) {
                links[i] = std::move(links[i - 1]);
            }
            links[pos].index = child->get_first_index();
            links[pos].child = child;
            child->parent = this;
            size++;
        }

        void delete_link(u32 pos) {
            PLY_ASSERT(!is_leaf);
            PLY_ASSERT(pos < size);
            u32 limit = size - 1;
            Link* links = get_links();
            for (u32 i = pos; i < limit; i++) {
                links[i] = std::move(links[i + 1]);
            }
            links[limit].~Link();
            size--;
            // FIXME: Don't bother updating the index if the Index type doesn't
            // reference items. We only really need this eg. when Index is a StringView
            // into a String owned by an item, as in SemaEntity::name_to_child.
            if (pos == 0 && limit > 0) {
                this->update_index_in_parent(links[0].index);
            }
        }

        u32 find_greater_or_equal_pos(const Index& index) {
            PLY_ASSERT(is_leaf);
            Item* items = get_items();
            u32 lo = 0;
            u32 hi = size;
            while (lo < hi) {
                u32 mid = (lo + hi) / 2;
                const Index& other_index = Traits::get_index(items[mid]);
                if (!Traits::less(other_index, index))
                    hi = mid;
                else
                    lo = mid + 1;
            }
            return lo;
        }

        u32 find_insert_pos(const Index& index) {
            // This is different from find_greater_or_equal_pos because, in the case of
            // CookJobSchedule, we want inserts with the same timestamp to appear
            // *after* the existing item, so that it acts as a queue.
            PLY_ASSERT(is_leaf);
            Item* items = get_items();
            u32 lo = 0;
            u32 hi = size;
            while (lo < hi) {
                u32 mid = (lo + hi) / 2;
                const Index& other_index = Traits::get_index(items[mid]);
                if (Traits::less(index, other_index))
                    hi = mid;
                else
                    lo = mid + 1;
            }
            return lo;
        }

        Item& insert_item_at(u32 pos) {
            PLY_ASSERT(is_leaf);
            PLY_ASSERT(size < Traits::NodeCapacity);
            PLY_ASSERT(pos <= size);
            Item* items = get_items();
            u32 s = size;
            new (items + s) Item();
            for (u32 i = s; i > pos; i--) {
                items[i] = std::move(items[i - 1]);
            }
            size++;
            return items[pos];
        }

        void update_index_in_parent(const Index& index) {
            if (this->parent) {
                u32 i = this->parent->find_link(this);
                this->parent->get_links()[i].index = index;
                if (i == 0) {
                    this->parent->update_index_in_parent(index);
                }
            }
        }

        void delete_item(u32 pos) {
            PLY_ASSERT(is_leaf);
            PLY_ASSERT(pos < size);
            u32 limit = size - 1;
            Item* items = get_items();
            for (u32 i = pos; i < limit; i++) {
                items[i] = std::move(items[i + 1]);
            }
            items[limit].~Item();
            size--;
            // FIXME: Don't bother updating the index if the Index type doesn't
            // reference items. We only really need this eg. when Index is a StringView
            // into a String owned by an item, as in SemaEntity::name_to_child.
            if (pos == 0 && limit > 0) {
                this->update_index_in_parent(Traits::get_index(items[0]));
            }
        }

        Node* split_leaf() {
            PLY_ASSERT(is_leaf);
            PLY_ASSERT(size >= 2);
            u32 s = size;
            u32 mid = s / 2;
            Node* right_sibling = create_leaf();
            Item* left_items = get_items();
            Item* right_items = right_sibling->get_items();
            for (u32 i = 0; i < s - mid; i++) {
                Item* src = left_items + (mid + i);
                Item* dst = right_items + i;
                new (dst) Item(std::move(*src));
                Traits::on_item_moved(*dst, right_sibling);
                src->~Item();
            }
            size = mid;
            right_sibling->size = (s - mid);
            return right_sibling;
        }

        Node* split_inner_node() {
            PLY_ASSERT(!is_leaf);
            PLY_ASSERT(size >= 2);
            u32 s = size;
            u32 mid = s / 2;
            Node* right_sibling = create_inner_node();
            Link* left_links = get_links();
            Link* right_links = right_sibling->get_links();
            for (u32 i = 0; i < s - mid; i++) {
                Link* src = left_links + (mid + i);
                Link* dst = right_links + i;
                new (dst) Link(std::move(*src));
                dst->child->parent = right_sibling;
                src->~Link();
            }
            size = mid;
            right_sibling->size = (s - mid);
            return right_sibling;
        }

        Node* merge(Node* right_sibling) {
            PLY_ASSERT(is_leaf == right_sibling->is_leaf);
            PLY_ASSERT(parent == right_sibling->parent);
            PLY_ASSERT(size + right_sibling->size <= Traits::NodeCapacity);
            u32 ls = size;
            u32 rs = right_sibling->size;
            if (is_leaf) {
                Item* left_items = get_items();
                Item* right_items = right_sibling->get_items();
                for (u32 i = 0; i < rs; i++) {
                    Item* src = right_items + i;
                    Item* dst = left_items + (ls + i);
                    new (dst) Item(std::move(*src));
                    Traits::on_item_moved(*dst, this);
                    src->~Item();
                }
            } else {
                Link* left_links = get_links();
                Link* right_links = right_sibling->get_links();
                for (u32 i = 0; i < rs; i++) {
                    Link* src = right_links + i;
                    Link* dst = left_links + (ls + i);
                    new (dst) Link(std::move(*src));
                    dst->child->parent = this;
                    src->~Link();
                }
            }
            size += rs;
            Heap.free_aligned(right_sibling);
            return this;
        }

        void destroy_recursively() {
            u32 s = size;
            if (is_leaf) {
                Item* items = get_items();
                for (u32 i = 0; i < s; i++) {
                    items[i].~Item();
                }
            } else {
                Link* links = get_links();
                for (u32 i = 0; i < s; i++) {
                    links[i].child->destroy_recursively();
                    links[i].~Link();
                }
            }
            Heap.free_aligned(this);
        }

#if PLY_BTREE_VALIDATE
        void validate(u32 parent_link_index) {
            PLY_ASSERT(size > 0);
            if (is_leaf) {
                // Items must be in increasing order
                Index limit = parent ? parent->get_links()[parent_link_index].index
                                     : Traits::get_index(get_items()[0]);
                for (u32 i = 0; i < size; i++) {
                    const Index& index = Traits::get_index(get_items()[i]);
                    PLY_ASSERT(limit <= index);
                    limit = index;
                }
            } else {
                // Links must be in increasing order
                Link* links = get_links();
                Index limit = parent ? parent->get_links()[parent_link_index].index
                                     : links[0].index;
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

    void insert_right_child(Node* node, Node* left_child, Node* right_child) {
        if (node) {
            PLY_ASSERT(!node->is_leaf);
            u32 right_pos = node->find_link(left_child) + 1;
            if (node->size >= Traits::NodeCapacity) {
                Node* right_sibling = node->split_inner_node();
                insert_right_child(node->parent, node, right_sibling);
                if (right_pos > node->size) {
                    right_pos -= node->size;
                    node = right_sibling;
                    PLY_ASSERT(right_pos <= node->size);
                }
            }
            node->insert_link(right_child, right_pos);
        } else {
            // Create new root node for both children
            PLY_ASSERT(m_root == left_child);
            Node* node = Node::create_inner_node();
            node->size = 2;
            Link* links = node->get_links();
            new (links + 0) Link{left_child->get_first_index(), left_child};
            left_child->parent = node;
            new (links + 1) Link{right_child->get_first_index(), right_child};
            right_child->parent = node;
            m_root = node;
        }
    }

    Node* split(Node* node) {
        PLY_ASSERT(node->is_leaf);
        Node* right_sibling = node->split_leaf();
        insert_right_child(node->parent, node, right_sibling);
        return right_sibling;
    }

    void try_compact(Node* node) {
        // Note: This BTree is not really self-balancing because we never "steal" from
        // siblings. We only ever merge two siblings into one. For example, it's
        // possible for the left-most or right-most child of any node to contain only a
        // single item or link (if it couldn't be merged with its sibling). If we wanted
        // self-balancing behavior, we would steal from a sibling any time a node became
        // less than half-full.
        Node* parent = node->parent;
        if (parent) {
            Link* links = parent->get_links();
            u32 pos = parent->find_link(node);
            if (node->size == 0) {
                Heap.free_aligned(node);
                parent->delete_link(pos);
                try_compact(parent);
            } else {
                if (pos > 0) {
                    Node* left_sibling = links[pos - 1].child;
                    PLY_ASSERT(node->is_leaf == left_sibling->is_leaf);
                    if (node->size + left_sibling->size <= Traits::NodeCapacity) {
                        node = left_sibling->merge(node);
                        parent->delete_link(pos);
                        try_compact(parent);
                        return;
                    }
                    // Fall through to other test
                }
                if (pos < u32(parent->size) - 1) {
                    Node* right_sibling = links[pos + 1].child;
                    PLY_ASSERT(node->is_leaf == right_sibling->is_leaf);
                    if (node->size + right_sibling->size <= Traits::NodeCapacity) {
                        node->merge(right_sibling);
                        parent->delete_link(pos + 1);
                        try_compact(parent);
                    }
                }
            }
        } else {
            PLY_ASSERT(node == m_root);
            if (node->size == 0) {
                clear();
            } else if (node->size == 1 && !node->is_leaf) {
                m_root = node->get_links()[0].child;
                m_root->parent = nullptr;
                node->delete_link(0);
                Heap.free_aligned(node);
            }
        }
    }

public:
    class Iterator {
    private:
        static constexpr uptr PosMask = uptr(Traits::NodeCapacity - 1);
        friend class BTree;
        uptr leaf_pos;

        Node* get_leaf() const {
            Node* leaf = (Node*) (leaf_pos & ~PosMask);
            PLY_ASSERT(leaf->is_leaf);
            return leaf;
        }

        u32 get_pos() const {
            return leaf_pos & PosMask;
        }

    public:
        Iterator() : leaf_pos{0} {
        }

        Iterator(Node* leaf, u32 pos) {
            PLY_ASSERT(is_aligned_power_of2((uptr) leaf, Traits::NodeCapacity));
            PLY_ASSERT(pos < Traits::NodeCapacity);
            leaf_pos = uptr(leaf) | pos;
        }

        // Note: the "end" of the tree is considered valid
        bool is_valid() const {
            return (leaf_pos != 0);
        }

        Item& get_item() const {
            PLY_ASSERT(is_valid());
            return get_leaf()->get_items()[get_pos()];
        }

        void next() {
            // Why don't we assert here?
            if (leaf_pos != 0) {
                Node* leaf = get_leaf();
                if (get_pos() + 1 < leaf->size) {
                    leaf_pos++;
                    return;
                }
                leaf_pos = (uptr) leaf->get_right_sibling(); // pos is 0
                PLY_ASSERT(is_aligned_power_of2(leaf_pos, Traits::NodeCapacity));
            }
        }

        void prev() {
            PLY_ASSERT(is_valid()); // possibly points beyond end, but that's OK
            if (get_pos() > 0) {
                leaf_pos--;
            } else {
                Node* left = get_leaf()->get_left_sibling();
                PLY_ASSERT(left->size > 0);
                leaf_pos = (uptr) left + (left->size - 1);
            }
        }

        bool operator!=(const Iterator& other) {
            return leaf_pos != other.leaf_pos;
        }

        void operator++() {
            next();
        }

        Item& operator*() const {
            return get_item();
        }

        Item* operator->() const {
            return &get_item();
        }
    };

private:
    void update_link_index(const Iterator& iter) {
        Node* node = iter.get_leaf();
        if (node->size > 0 && iter.get_pos() == 0) {
            Node* parent = node->parent;
            if (parent) {
                PLY_ASSERT(!parent->is_leaf);
                const Index& new_index = Traits::get_index(node->get_items()[0]);
                u32 link_pos;
                do {
                    link_pos = parent->find_link(node);
                    //                    PLY_ASSERT(link_pos == 0 || new_index >=
                    //                    parent->get_links()[link_pos - 1].index);
                    PLY_ASSERT(link_pos == 0 ||
                               !Traits::less(new_index,
                                             parent->get_links()[link_pos - 1].index));
                    //                    PLY_ASSERT(link_pos == parent->size - 1 ||
                    //                    new_index <= parent->get_links()[link_pos +
                    //                    1].index);
                    PLY_ASSERT(link_pos == (u32) parent->size - 1 ||
                               !Traits::less(parent->get_links()[link_pos + 1].index,
                                             new_index));
                    Index& link_index = parent->get_links()[link_pos].index;
                    if (new_index == link_index)
                        return;
                    link_index = new_index;
                    node = parent;
                    parent = node->parent;
                } while (parent && link_pos == 0);
            }
        }
    }

public:
    BTree() {
        PLY_ASSERT(is_power_of2(Traits::NodeCapacity));
    }

    BTree(BTree&& other) : m_root{other.m_root} {
        other.m_root = nullptr;
    }

    ~BTree() {
        if (m_root)
            m_root->destroy_recursively();
    }

    bool is_empty() const {
        return (m_root == nullptr);
    }

    void clear() {
        if (m_root) {
            m_root->destroy_recursively();
            m_root = nullptr;
        }
    }

    Iterator find_last_less_than(const Index& index) const {
        if (!m_root)
            return {nullptr, 0};
        Node* node = m_root;
        while (!node->is_leaf) {
            node = node->get_last_child_less_than(index);
        }
        u32 pos = node->find_greater_or_equal_pos(index);
        if (pos > 0) {
            return {node, pos - 1};
        } else {
            node = node->get_left_sibling();
            return {node, node ? node->size - 1 : 0u};
        }
    }

    Iterator find_first_greater_or_equal_to(const Index& index) const {
        if (!m_root)
            return {nullptr, 0};
        Node* node = m_root;
        while (!node->is_leaf) {
            node = node->get_last_child_less_than(index);
        }
        u32 pos = node->find_greater_or_equal_pos(index);
        if (pos >= node->size)
            return {node->get_right_sibling(), 0};
        return {node, pos};
    }

    Iterator prepare_insert(const Index& index) {
        if (!m_root)
            m_root = Node::create_leaf();
        Node* node = m_root;
        while (!node->is_leaf) {
            node = node->get_last_child_less_than(index);
        }
        u32 pos = node->find_insert_pos(index);
        if (node->size >= Traits::NodeCapacity) {
            Node* right_sibling = split(node);
            if (pos > node->size) {
                pos -= node->size;
                node = right_sibling;
            }
        }
        return Iterator{node, pos};
    }

    void insert(const Item& item) {
        Iterator iter = prepare_insert(Traits::get_index(item));
        Item& dst_item = iter.get_leaf()->insert_item_at(iter.get_pos());
        dst_item = item;
        Traits::on_item_moved(dst_item, iter.get_leaf());
        update_link_index(iter);
    }

    void insert(Item&& item) {
        Iterator iter = prepare_insert(Traits::get_index(item));
        Item& dst_item = iter.get_leaf()->insert_item_at(iter.get_pos());
        dst_item = std::move(item);
        Traits::on_item_moved(dst_item, iter.get_leaf());
        update_link_index(iter);
    }

    // locate() is used when items maintain pointers back to their own Nodes, such as in
    // CookJobSchedule. "context" is just a pointer to the Node with the type erased.
    // (In CookJobSchedule, this back-pointer is maintained in Job::bt_context, and it
    // gets updated each time Traits::on_item_moved() is called.) (This avoids having to
    // search for the Job in the CookJobSchedule from scratch each time. Maybe this
    // strategy is a little bit overkill and it would be OK to just search for the Job
    // from scratch each time... I'll leave it as-is for now.)
    template <typename MatchItem>
    Iterator locate(void* context, const MatchItem& item) {
        Node* in_leaf = (Node*) context;
        PLY_ASSERT(in_leaf->is_leaf);
        u32 s = in_leaf->size;
        Item* items = in_leaf->get_items();
        for (u32 i = 0; i < s; i++) {
            if (Traits::equal(items[i], item))
                return Iterator{in_leaf, i};
        }
        PLY_ASSERT(0); // Shouldn't get here
        return Iterator{nullptr, 0};
    }

    void check_parents(Iterator& iter) {
        PLY_ASSERT(!is_empty());
        Node* node = iter.get_leaf();
        PLY_ASSERT(node->size > iter.get_pos());
        Index index = Traits::get_index(node->get_items()[iter.get_pos()]);
        while (node->parent) {
            u32 pos = node->parent->find_link(node);
            const Index& link_index = node->parent->get_links()[pos].index;
            PLY_ASSERT(link_index <= index);
            node = node->parent;
            index = link_index;
        }
    }

    Item remove(Iterator& iter) {
        PLY_ASSERT(!is_empty());
        Node* node = iter.get_leaf();
        PLY_ASSERT(node->size > iter.get_pos());
        Item* items = node->get_items();
        Item result = std::move(items[iter.get_pos()]);
        node->delete_item(iter.get_pos());
        update_link_index(iter);
        try_compact(node);
        return result;
    }

    // FIXME: Support const iteration
    Iterator begin() {
        Node* node = m_root;
        if (node) {
            while (!node->is_leaf) {
                PLY_ASSERT(node->size > 0);
                node = node->get_links()[0].child;
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
        PLY_ASSERT(!is_empty());
        return begin().get_item();
    }

    Item pop_front() {
        PLY_ASSERT(!is_empty());
        Iterator iter = begin();
        return remove(iter);
    }

#if PLY_BTREE_VALIDATE
    void validate() {
        if (m_root) {
            PLY_ASSERT(m_root->parent == nullptr);
            m_root->validate(0);
            // Ensure all items are in increasing order
            Index limit = m_root->is_leaf ? Traits::get_index(m_root->get_items()[0])
                                          : m_root->get_links()[0].index;
            for (const Item& item : *this) {
                const Index& index = Traits::get_index(item);
                PLY_ASSERT(limit <= index);
                limit = index;
            }
        }
    }
#endif
};

} // namespace ply
