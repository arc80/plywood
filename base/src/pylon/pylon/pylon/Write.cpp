/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <pylon/Core.h>
#include <pylon/Write.h>
#include <ply-runtime/Algorithm.h>

namespace pylon {

struct WriteContext {
    OutStream& out;
    int depth = 0;

    PLY_INLINE WriteContext(OutStream& out) : out{out} {
    }

    PLY_NO_INLINE void indent() {
        for (int i = 0; i < depth; i++) {
            this->out << "  ";
        }
    }

    PLY_NO_INLINE void write(const Node* aNode) {
        if (aNode->isObject()) {
            this->out << "{\n";
            this->depth++;
            const Node::Object& objNode = aNode->object();
            u32 numItems = objNode.items.numItems();
            for (u32 i : range(numItems)) {
                const Node::Object::Item& objItem = objNode.items[i];
                indent();
                this->out.format("\"{}\": ", escape(objItem.key));
                write(objItem.value);
                if (i + 1 < numItems) {
                    this->out << ',';
                }
                this->out << '\n';
            }
            this->depth--;
            indent();
            this->out << '}';
        } else if (aNode->isArray()) {
            this->out << "[\n";
            this->depth++;
            ArrayView<const Node* const> arrNode = aNode->arrayView();
            u32 numItems = arrNode.numItems;
            for (u32 i : range(numItems)) {
                indent();
                write(arrNode[i]);
                if (i + 1 < numItems) {
                    this->out << ',';
                }
                this->out << '\n';
            }
            this->depth--;
            indent();
            this->out << ']';
        } else if (aNode->isText()) {
            this->out.format("\"{}\"", escape(aNode->text()));
        } else {
            PLY_ASSERT(0);  // unsupported
        }
    }
};

PLY_NO_INLINE void write(OutStream& out, const Node* aNode) {
    WriteContext ctx{out};
    ctx.write(aNode);
}

PLY_NO_INLINE String toString(const Node* aNode) {
    MemOutStream out;
    write(out, aNode);
    return out.moveToString();
}

} // namespace pylon
