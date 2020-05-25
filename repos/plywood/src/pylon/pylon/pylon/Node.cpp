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
