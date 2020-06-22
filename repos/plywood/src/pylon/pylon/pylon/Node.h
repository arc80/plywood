/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <pylon/Core.h>

namespace pylon {

struct Location {
    u32 line;
    u32 column;
};

struct Node {
    struct Object {
        struct Item; // Defined further below
        struct IndexTraits {
            using Key = StringView;
            using Item = u32;
            using Context = Array<Object::Item>;
            static StringView comparand(u32 item, const Array<Object::Item>& ctx);
        };
        HashMap<IndexTraits> index;
        Array<Item> items;

        PLY_INLINE Object() = default;
        PLY_INLINE Object(Object&& other) = default;
        PLY_NO_INLINE Object(const Object& other);
        PLY_INLINE void operator=(Object&& other);
        PLY_INLINE void operator=(const Object& other);

        Node& insertOrFind(HybridString&& name);
        Node& find(StringView name);
        const Node& find(StringView name) const;
    };

    struct Type {
        // ply make switch
        struct Invalid { //
        };
        struct Object {
            Node::Object obj;
        };
        struct Array {
            ply::Array<Node> arr;
        };
        struct Text {
            HybridString str;
        };
#include "codegen/switch-pylon-Node-Type.inl" //@@ply
    };
    Type type;
    Location location = {0, 0};

    static Object emptyObject;
    static ply::Array<Node> emptyArray;
    static Node invalidNode;

    PLY_INLINE bool isValid() const {
        return type.id != Type::ID::Invalid;
    }

    static PLY_INLINE Node createObject(const Location& location) {
        Node node;
        node.type.object().switchTo();
        node.location = location;
        return node;
    }

    static PLY_INLINE Node createArray(const Location& location) {
        Node node;
        node.type.array().switchTo();
        node.location = location;
        return node;
    }

    static PLY_INLINE Node createText(HybridString&& str, const Location& location) {
        Node node;
        auto text = node.type.text().switchTo();
        text->str = std::move(str);
        node.location = location;
        return node;
    }

    //-----------------------------------------------------------
    // Object
    //-----------------------------------------------------------

    PLY_INLINE bool isObject() const {
        return type.id == Type::ID::Object;
    }

    PLY_INLINE const Object& object() const {
        if (auto object = type.object()) {
            return object->obj;
        } else {
            return emptyObject;
        }
    }

    PLY_INLINE Object& object() {
        PLY_ASSERT(type.id == Type::ID::Object);
        return type.object()->obj;
    }

    Node& operator[](StringView key);
    const Node& operator[](StringView key) const;

    //-----------------------------------------------------------
    // Array
    //-----------------------------------------------------------

    PLY_INLINE bool isArray() const {
        return type.id == Type::ID::Array;
    }

    PLY_INLINE const ply::Array<Node>& array() const {
        if (auto array = type.array()) {
            return array->arr;
        } else {
            return emptyArray;
        }
    }

    PLY_INLINE ply::Array<Node>& array() {
        PLY_ASSERT(type.id == Type::ID::Array);
        return type.array()->arr;
    }

    PLY_INLINE u32 numItems() const {
        if (auto array = type.array()) {
            return array->arr.numItems();
        }
        return 0;
    }

    PLY_INLINE const Node& operator[](u32 index) const {
        if (auto array = type.array()) {
            if (index < array->arr.numItems()) {
                return array->arr[index];
            }
        }
        return invalidNode;
    }

    PLY_INLINE Node& operator[](u32 index) {
        if (auto array = type.array()) {
            if (index < array->arr.numItems()) {
                return array->arr[index];
            }
        }
        return invalidNode;
    }

    PLY_INLINE const Node* begin() const {
        return array().begin();
    }

    PLY_INLINE const Node* end() const {
        return array().end();
    }

    PLY_INLINE Node* begin() {
        return array().begin();
    }

    PLY_INLINE Node* end() {
        return array().end();
    }

    //-----------------------------------------------------------
    // Text
    //-----------------------------------------------------------

    PLY_INLINE bool isText() const {
        return type.id == Type::ID::Text;
    }

    PLY_INLINE StringView text() const {
        if (auto text = type.text()) {
            return text->str.view();
        } else {
            return {};
        }
    }

    PLY_INLINE operator StringView() const {
        return text();
    }

    bool isNumeric() const;
    double numeric() const;

    template <typename T>
    PLY_INLINE T numeric() const {
        return (T) this->numeric();
    }
};

struct Node::Object::Item {
    HybridString name;
    Node value;
};

PLY_INLINE void Node::Object::operator=(Object&& other) {
    this->~Object();
    new (this) Object{std::move(other)};
}

PLY_INLINE void Node::Object::operator=(const Object& other) {
    this->~Object();
    new (this) Object{other};
}

PLY_INLINE StringView Node::Object::IndexTraits::comparand(u32 item,
                                                           const Array<Object::Item>& ctx) {
    return ctx[item].name.view();
}

} // namespace pylon
