/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <pylon/Core.h>

namespace pylon {

struct Node {
    enum class Type {
        Invalid = 0,
        Text,
        Array,
        Object,
    };

    u64 type : 4;
    // StringView and FileLocationMap don't support files greater than 4GB yet, even though we
    // support large file offsets here:
    u64 fileOfs : 60;

    struct Object {
        struct Item {
            HybridString key;
            Owned<Node> value;
        };

        struct IndexTraits {
            using Key = StringView;
            using Item = u32;
            using Context = Array<Object::Item>;
            static PLY_INLINE StringView comparand(u32 item, const Array<Object::Item>& ctx) {
                return ctx[item].key.view();
            }
        };

        HashMap<IndexTraits> index;
        Array<Item> items;
    };

    union {
        HybridString text_;
        Array<Owned<Node>> array_;
        Object object_;
    };

    Node() = delete; // use create functions
    PLY_NO_INLINE ~Node();

    Owned<Node> copy() const;

    static PLY_NO_INLINE Owned<Node> createInvalid();
    static PLY_NO_INLINE Owned<Node> allocText();
    static PLY_NO_INLINE Owned<Node> createArray(u64 fileOfs = 0);
    static PLY_NO_INLINE Owned<Node> createObject(u64 fileOfs = 0);

    static PLY_INLINE Owned<Node> createText(HybridString&& text, u64 fileOfs = 0) {
        Owned<Node> node = allocText();
        node->type = (u64) Type::Text;
        node->fileOfs = fileOfs;
        new (&node->text_) decltype(node->text_){std::move(text)};
        return node;
    }

    static u64 InvalidNodeHeader;
    static Object EmptyObject;

    PLY_INLINE bool isValid() const {
        return this->type != (u64) Type::Invalid;
    }

    //-----------------------------------------------------------
    // Text
    //-----------------------------------------------------------

    PLY_INLINE bool isText() const {
        return this->type == (u64) Type::Text;
    }

    PLY_NO_INLINE Tuple<bool, double> numeric() const;

    PLY_INLINE StringView text() const {
        if (this->type == (u64) Type::Text) {
            return this->text_;
        } else {
            return {};
        }
    }

    PLY_INLINE void setText(HybridString&& text) {
        if (this->type == (u64) Type::Text) {
            this->text_ = std::move(text);
        }
    }

    //-----------------------------------------------------------
    // Array
    //-----------------------------------------------------------

    PLY_INLINE bool isArray() const {
        return this->type == (u64) Type::Array;
    }

    PLY_INLINE Borrowed<Node> get(u32 i) {
        if (this->type != (u64) Type::Array)
            return (Node*) &InvalidNodeHeader;
        if (i >= this->array_.numItems())
            return (Node*) &InvalidNodeHeader;
        return this->array_[i].borrow();
    }

    PLY_INLINE const Node* get(u32 i) const {
        return const_cast<Node*>(this)->get(i);
    }

    PLY_INLINE ArrayView<const Node* const> arrayView() const {
        if (this->type == (u64) Type::Array) {
            return reinterpret_cast<const Array<const Node*>&>(this->array_).view();
        } else {
            return {};
        }
    }

    PLY_INLINE Array<Owned<Node>>& array() {
        PLY_ASSERT(this->type == (u64) Type::Array);
        return this->array_;
    }

    //-----------------------------------------------------------
    // Object
    //-----------------------------------------------------------

    PLY_INLINE bool isObject() const {
        return this->type == (u64) Type::Object;
    }

    PLY_NO_INLINE Borrowed<Node> get(StringView key);
    PLY_INLINE const Node* get(StringView key) const {
        return const_cast<Node*>(this)->get(key);
    }
    PLY_NO_INLINE void set(HybridString&& key, Owned<Node>&& value);
    PLY_NO_INLINE Owned<Node> remove(StringView key);

    PLY_INLINE const Object& object() const {
        if (this->type == (u64) Type::Object) {
            return this->object_;
        } else {
            return EmptyObject;
        }
    }

    PLY_INLINE Object& object() {
        PLY_ASSERT(this->type == (u64) Type::Object);
        return this->object_;
    }
};

} // namespace pylon
