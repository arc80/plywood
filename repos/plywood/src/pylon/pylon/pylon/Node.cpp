/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <pylon/Core.h>
#include <pylon/Node.h>

namespace pylon {

Node::Object Node::emptyObject;
ply::Array<Node> Node::emptyArray;
Node Node::invalidNode;

bool Node::isNumeric() const {
    StringView strView = type.text()->str.view();
    char* end = nullptr;
    // FIXME: Replace strtod with something that respects strView.numBytes (like TextInput):
    strtod(strView.bytes, &end);
    return (end == strView.bytes + strView.numBytes);
}

double Node::numeric() const {
    StringView strView = type.text()->str.view();
    char* end = nullptr;
    // FIXME: Replace strtod with something that respects strView.numBytes:
    double value = strtod(strView.bytes, &end);
    if (end == strView.bytes + strView.numBytes)
        return value;
    else
        return 0;
}

PLY_NO_INLINE Node::Object::Object(const Node::Object& other) : items{other.items} {
    // FIXME: Make HashMap copy operation and use that instead
    for (u32 i = 0; i < this->items.numItems(); i++) {
        *this->index.insertOrFind(this->items[i].name, &this->items) = i;
    }
}

Node& Node::Object::find(StringView name) {
    auto foundCursor = this->index.find(name, &this->items);
    if (foundCursor.wasFound()) {
        return this->items[*foundCursor].value;
    }
    return Node::invalidNode;
}

const Node& Node::Object::find(StringView name) const {
    auto foundCursor = this->index.find(name, &this->items);
    if (foundCursor.wasFound()) {
        return this->items[*foundCursor].value;
    }
    return Node::invalidNode;
}

PLY_NO_INLINE Node& Node::Object::insertOrFind(HybridString&& name) {
    u32 index = this->items.numItems();
    auto cursor = this->index.insertOrFind(name, &this->items);
    if (cursor.wasFound()) {
        return this->items[*cursor].value;
    } else {
        *cursor = index;
        Item& objItem = this->items.append();
        objItem.name = std::move(name);
        return objItem.value;
    }
}

Node& Node::operator[](StringView key) {
    if (auto object = type.object()) {
        return object->obj.find(key);
    }
    return invalidNode;
}

const Node& Node::operator[](StringView key) const {
    if (auto object = type.object()) {
        return object->obj.find(key);
    }
    return invalidNode;
}

} // namespace pylon

#include "codegen/Node.inl" //%%
