/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <pylon/Core.h>
#include <pylon/Write.h>

namespace pylon {

struct WriteContext {
    StringWriter* sw;
    int depth = 0;

    PLY_INLINE WriteContext(OutStream* outs) : sw{outs->strWriter()} {
    }

    PLY_NO_INLINE void indent() {
        for (int i = 0; i < depth; i++) {
            *this->sw << "  ";
        }
    }

    PLY_NO_INLINE void write(const Node& aNode) {
        if (aNode.isObject()) {
            *this->sw << "{\n";
            this->depth++;
            const Node::Object& objNode = aNode.object();
            u32 numItems = objNode.items.numItems();
            for (u32 i : range(numItems)) {
                const Node::Object::Item& objItem = objNode.items[i];
                indent();
                this->sw->format("\"{}\": ", fmt::EscapedString{objItem.name});
                write(objItem.value);
                if (i + 1 < numItems) {
                    *this->sw << ',';
                }
                *this->sw << '\n';
            }
            this->depth--;
            indent();
            *this->sw << '}';
        } else if (aNode.isArray()) {
            *this->sw << "[\n";
            this->depth++;
            const ply::Array<Node>& arrNode = aNode.array();
            u32 numItems = arrNode.numItems();
            for (u32 i : range(numItems)) {
                indent();
                write(arrNode[i]);
                if (i + 1 < numItems) {
                    *this->sw << ',';
                }
                *this->sw << '\n';
            }
            this->depth--;
            indent();
            *this->sw << ']';
        } else if (aNode.isText()) {
            this->sw->format("\"{}\"", fmt::EscapedString{aNode.text()});
        }
    }
};

PLY_NO_INLINE void write(OutStream* outs, const Node& aNode) {
    WriteContext ctx{outs};
    ctx.write(aNode);
}

PLY_NO_INLINE String toString(const Node& aNode) {
    StringWriter sw;
    write(&sw, aNode);
    return sw.moveToString();
}

} // namespace pylon
