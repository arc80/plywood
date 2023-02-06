/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <pylon/Core.h>
#include <pylon/Write.h>

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

    PLY_NO_INLINE void write(const Node* a_node) {
        if (a_node->is_object()) {
            this->out << "{\n";
            this->depth++;
            const Node::Object& obj_node = a_node->object();
            u32 num_items = obj_node.items.num_items();
            for (u32 i = 0; i < num_items; i++) {
                const Node::Object::Item& obj_item = obj_node.items[i];
                indent();
                this->out.format("\"{}\": ", escape(obj_item.key));
                write(obj_item.value);
                if (i + 1 < num_items) {
                    this->out << ',';
                }
                this->out << '\n';
            }
            this->depth--;
            indent();
            this->out << '}';
        } else if (a_node->is_array()) {
            this->out << "[\n";
            this->depth++;
            ArrayView<const Node* const> arr_node = a_node->array_view();
            u32 num_items = arr_node.num_items;
            for (u32 i = 0; i < num_items; i++) {
                indent();
                write(arr_node[i]);
                if (i + 1 < num_items) {
                    this->out << ',';
                }
                this->out << '\n';
            }
            this->depth--;
            indent();
            this->out << ']';
        } else if (a_node->is_text()) {
            this->out.format("\"{}\"", escape(a_node->text()));
        } else {
            PLY_ASSERT(0); // unsupported
        }
    }
};

PLY_NO_INLINE void write(OutStream& out, const Node* a_node) {
    WriteContext ctx{out};
    ctx.write(a_node);
}

PLY_NO_INLINE String to_string(const Node* a_node) {
    MemOutStream out;
    write(out, a_node);
    return out.move_to_string();
}

} // namespace pylon
