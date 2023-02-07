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

Node::~Node() {
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
            return create_text(this->text_.view(), this->file_ofs);
        }

        case Type::Array: {
            Owned<Node> dst = create_array(this->file_ofs);
            dst->array_.resize(this->array_.num_items());
            for (u32 i = 0; i < this->array_.num_items(); i++) {
                dst->array_[i] = this->array_[i]->copy();
            }
            return dst;
        }

        case Type::Object: {
            Owned<Node> dst = create_object(this->file_ofs);
            dst->object_.items.resize(this->object_.items.num_items());
            for (u32 i = 0; i < this->object_.items.num_items(); i++) {
                Object::Item& dst_item = dst->object_.items[i];
                dst_item.key = this->object_.items[i].key.view();
                dst_item.value = this->object_.items[i].value->copy();
                *dst->object_.index.insert_or_find(dst_item.key) = i;
            }
            return dst;
        }

        default: {
            return create_invalid();
        }
    }
}

Owned<Node> Node::create_invalid() {
    Owned<Node> node = (Node*) Heap.alloc(PLY_MEMBER_OFFSET(Node, text_));
    node->type = (u64) Type::Invalid;
    node->file_ofs = 0;
    return node;
}

Owned<Node> Node::alloc_text() {
    return (Node*) Heap.alloc(PLY_MEMBER_OFFSET(Node, text_) + sizeof(Node::text_));
}

Owned<Node> Node::create_array(u64 file_ofs) {
    Owned<Node> node =
        (Node*) Heap.alloc(PLY_MEMBER_OFFSET(Node, array_) + sizeof(Node::array_));
    node->type = (u64) Type::Array;
    node->file_ofs = file_ofs;
    new (&node->array_) decltype(node->array_);
    return node;
}

Owned<Node> Node::create_object(u64 file_ofs) {
    Owned<Node> node =
        (Node*) Heap.alloc(PLY_MEMBER_OFFSET(Node, object_) + sizeof(Node::object_));
    node->type = (u64) Type::Object;
    node->file_ofs = file_ofs;
    new (&node->object_) decltype(node->object_);
    return node;
}

Tuple<bool, double> Node::numeric() const {
    if (this->type != (u64) Type::Text)
        return {false, 0.0};

    ViewInStream vins{this->text_};
    double value = vins.parse<double>();
    return {!vins.any_parse_error(), value};
}

Node* Node::get(StringView key) {
    if (this->type != (u64) Type::Object)
        return (Node*) &InvalidNodeHeader;

    auto cursor = this->object_.index.find(key, &this->object_.items);
    if (!cursor.was_found())
        return (Node*) &InvalidNodeHeader;

    return this->object_.items[*cursor].value;
}

Node* Node::set(HybridString&& key, Owned<Node>&& value) {
    Node* result = value;
    if (this->type != (u64) Type::Object)
        return result;

    auto cursor = this->object_.index.insert_or_find(key.view(), &this->object_.items);
    if (cursor.was_found()) {
        this->object_.items[*cursor].value = std::move(value);
    } else {
        *cursor = this->object_.items.num_items();
        this->object_.items.append({std::move(key), std::move(value)});
    }
    return result;
}

Owned<Node> Node::remove(StringView key) {
    if (this->type != (u64) Type::Object)
        return nullptr;

    auto cursor = this->object_.index.find(key, &this->object_.items);
    if (!cursor.was_found())
        return nullptr;

    u32 index = *cursor;
    cursor.erase();
    Owned<Node> result = std::move(this->object_.items[index].value);
    this->object_.items.erase(index);
    for (u32& to_adjust : this->object_.index) {
        if (to_adjust > index) {
            to_adjust--;
        }
    }
    return result;
}

} // namespace pylon
