/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <pylon/Core.h>
#include <pylon/Write.h>

namespace pylon {

struct WriteContext {
    OutStream* outs;
    int depth = 0;

    PLY_INLINE WriteContext(OutStream* outs) : outs{outs } {
    }

    PLY_NO_INLINE void indent() {
        for (int i = 0; i < depth; i++) {
            *this->outs << "  ";
        }
    }

    PLY_NO_INLINE void write(const Node* aNode) {
        if (aNode->isObject()) {
            *this->outs << "{\n";
            this->depth++;
            const Node::Object& objNode = aNode->object();
            u32 numItems = objNode.items.numItems();
            for (u32 i : range(numItems)) {
                const Node::Object::Item& objItem = objNode.items[i];
                indent();
                this->outs->format("\"{}\": ", fmt::EscapedString{objItem.key});
                write(objItem.value);
                if (i + 1 < numItems) {
                    *this->outs << ',';
                }
                *this->outs << '\n';
            }
            this->depth--;
            indent();
            *this->outs << '}';
        } else if (aNode->isArray()) {
            *this->outs << "[\n";
            this->depth++;
            ArrayView<const Node* const> arrNode = aNode->arrayView();
            u32 numItems = arrNode.numItems;
            for (u32 i : range(numItems)) {
                indent();
                write(arrNode[i]);
                if (i + 1 < numItems) {
                    *this->outs << ',';
                }
                *this->outs << '\n';
            }
            this->depth--;
            indent();
            *this->outs << ']';
        } else if (aNode->isText()) {
            this->outs->format("\"{}\"", fmt::EscapedString{aNode->text()});
        } else {
            PLY_ASSERT(0);  // unsupported
        }
    }
};

PLY_NO_INLINE void write(OutStream* outs, const Node* aNode) {
    WriteContext ctx{outs};
    ctx.write(aNode);
}

PLY_NO_INLINE String toString(const Node* aNode) {
    MemOutStream mout;
    write(&mout, aNode);
    return mout.moveToString();
}

} // namespace pylon
