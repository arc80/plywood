/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <pylon/Core.h>
#include <pylon/Node.h>

namespace pylon {

u64 Node::InvalidNodeHeader = 0;
Node::Object Node::EmptyObject;

PLY_NO_INLINE Node::~Node() {
    switch ((Type) this->type) {
        case Type::Text: {
            destruct(this->text_);
            break;
        }

        case Type::Array: {
            destruct(this->array_);
            break;
        }

        case Type::Object: {
            destruct(this->object_);
            break;
        }

        default:
            break;
    }
}

Owned<Node> Node::copy() const {
    switch ((Type) this->type) {
        case Type::Text: {
            return createText(this->text_.view(), this->fileOfs);
        }

        case Type::Array: {
            Owned<Node> dst = createArray(this->fileOfs);
            dst->array_.resize(this->array_.numItems());
            for (u32 i = 0; i < this->array_.numItems(); i++) {
                dst->array_[i] = this->array_[i]->copy();
            }
            return dst;
        }

        case Type::Object: {
            Owned<Node> dst = createObject(this->fileOfs);
            dst->object_.items.resize(this->object_.items.numItems());
            for (u32 i = 0; i < this->object_.items.numItems(); i++) {
                Object::Item& dstItem = dst->object_.items[i];
                dstItem.key = this->object_.items[i].key.view();
                dstItem.value = this->object_.items[i].value->copy();
                *dst->object_.index.insertOrFind(dstItem.key) = i;
            }
            return dst;
        }

        default: {
            return createInvalid();
        }
    }
}

PLY_NO_INLINE Owned<Node> Node::createInvalid() {
    Owned<Node> node = (Node*) Heap.alloc(PLY_MEMBER_OFFSET(Node, text_));
    node->type = (u64) Type::Invalid;
    node->fileOfs = 0;
    return node;
}

PLY_NO_INLINE Owned<Node> Node::allocText() {
    return (Node*) Heap.alloc(PLY_MEMBER_OFFSET(Node, text_) + sizeof(Node::text_));
}

PLY_NO_INLINE Owned<Node> Node::createArray(u64 fileOfs) {
    Owned<Node> node =
        (Node*) Heap.alloc(PLY_MEMBER_OFFSET(Node, array_) + sizeof(Node::array_));
    node->type = (u64) Type::Array;
    node->fileOfs = fileOfs;
    new (&node->array_) decltype(node->array_);
    return node;
}

PLY_NO_INLINE Owned<Node> Node::createObject(u64 fileOfs) {
    Owned<Node> node =
        (Node*) Heap.alloc(PLY_MEMBER_OFFSET(Node, object_) + sizeof(Node::object_));
    node->type = (u64) Type::Object;
    node->fileOfs = fileOfs;
    new (&node->object_) decltype(node->object_);
    return node;
}

PLY_NO_INLINE Tuple<bool, double> Node::numeric() const {
    if (this->type != (u64) Type::Text)
        return {false, 0.0};

    ViewInStream vins{this->text_};
    double value = vins.parse<double>();
    return {!vins.anyParseError(), value};
}

PLY_NO_INLINE Node* Node::get(StringView key) {
    if (this->type != (u64) Type::Object)
        return (Node*) &InvalidNodeHeader;

    auto cursor = this->object_.index.find(key, &this->object_.items);
    if (!cursor.wasFound())
        return (Node*) &InvalidNodeHeader;

    return this->object_.items[*cursor].value;
}

PLY_NO_INLINE Node* Node::set(HybridString&& key, Owned<Node>&& value) {
    Node* result = value;
    if (this->type != (u64) Type::Object)
        return result;

    auto cursor = this->object_.index.insertOrFind(key.view(), &this->object_.items);
    if (cursor.wasFound()) {
        this->object_.items[*cursor].value = std::move(value);
    } else {
        *cursor = this->object_.items.numItems();
        this->object_.items.append({std::move(key), std::move(value)});
    }
    return result;
}

PLY_NO_INLINE Owned<Node> Node::remove(StringView key) {
    if (this->type != (u64) Type::Object)
        return nullptr;

    auto cursor = this->object_.index.find(key, &this->object_.items);
    if (!cursor.wasFound())
        return nullptr;

    u32 index = *cursor;
    cursor.erase();
    Owned<Node> result = std::move(this->object_.items[index].value);
    this->object_.items.erase(index);
    for (u32& toAdjust : this->object_.index) {
        if (toAdjust > index) {
            toAdjust--;
        }
    }
    return result;
}

} // namespace pylon
