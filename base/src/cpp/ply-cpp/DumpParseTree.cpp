/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-cpp/Core.h>
#include <ply-cpp/Parser.h>
#include <ply-cpp/Grammar.h>
#include <ply-cpp/ErrorFormatting.h>

namespace ply {
namespace cpp {

void dump_parse_tree(OutStream& out, AnyObject any, u32 indent,
                     const PPVisitedFiles* visited_files) {
    auto do_indent = [&]() {
        for (u32 i = 0; i < indent; i++) {
            out << "  ";
        }
    };
    indent++;
    TypeKey* type_key = any.type->type_key;
    if (type_key == TypeDescriptor_Struct::type_key) {
        const TypeDescriptor_Struct* struct_type =
            any.type->cast<TypeDescriptor_Struct>();
        if (struct_type->name == "ply::cpp::Token") {
            const Token* token = (const Token*) any.data;
            if (token->linear_loc >= 0 && visited_files) {
                out.format(
                    "{}: ",
                    expand_file_location(visited_files, token->linear_loc).to_string());
            }
            out.format("\"{}\"\n", escape(token->identifier));
        } else if (struct_type->name == "ply::cpp::grammar::QualifiedID") {
            out.format("\"{}\"", ((grammar::QualifiedID*) any.data)->to_string());
            out << '\n';
        } else {
            out << struct_type->name << '\n';
            for (const TypeDescriptor_Struct::Member& member : struct_type->members) {
                do_indent();
                out.format("{}: ", member.name);
                AnyObject any_member{PLY_PTR_OFFSET(any.data, member.offset),
                                     member.type};
                dump_parse_tree(out, any_member, indent, visited_files);
            }
        }
    } else if (type_key == TypeDescriptor_Enum::type_key) {
        const TypeDescriptor_Enum* enum_type = any.type->cast<TypeDescriptor_Enum>();
        u32 run_time_value = 0;
        if (enum_type->fixed_size == 1) {
            run_time_value = *(u8*) any.data;
        } else if (enum_type->fixed_size == 2) {
            run_time_value = *(u16*) any.data;
        } else if (enum_type->fixed_size == 4) {
            run_time_value = *(u32*) any.data;
        } else {
            PLY_ASSERT(0);
        }
        out.format("{}::{}\n", enum_type->name,
                   enum_type->find_value(run_time_value)->name);
    } else if (type_key == TypeDescriptor_Array::type_key) {
        TypeDescriptor_Array* array_type = any.type->cast<TypeDescriptor_Array>();
        TypeDescriptor* item_type = array_type->item_type;
        out << "[]\n";
        BaseArray* arr = (BaseArray*) any.data;
        void* item = arr->items;
        for (u32 i = 0; i < arr->num_items; i++) {
            do_indent();
            out.format("[{}] ", i);
            dump_parse_tree(out, AnyObject{item, item_type}, indent, visited_files);
            item = PLY_PTR_OFFSET(item, item_type->fixed_size);
        }
    } else if (type_key == TypeDescriptor_Switch::type_key) {
        TypeDescriptor_Switch* switch_type = any.type->cast<TypeDescriptor_Switch>();
        u16 id = *(u16*) any.data;
        void* buf = PLY_PTR_OFFSET(any.data, switch_type->storage_offset);
        dump_parse_tree(out, AnyObject{buf, switch_type->states[id].struct_type},
                        indent - 1, visited_files);
    } else if (type_key == TypeDescriptor_Owned::type_key) {
        TypeDescriptor_Owned* owned_type = any.type->cast<TypeDescriptor_Owned>();
        AnyObject target_obj = AnyObject{*(void**) any.data, owned_type->target_type};
        if (target_obj.data) {
            dump_parse_tree(out, target_obj, indent - 1, visited_files);
        } else {
            out << "(null)\n";
        }
    } else if (type_key == &TypeKey_Bool) {
        out << (*(bool*) any.data ? "true\n" : "false\n");
    } else {
        PLY_ASSERT(0);
    }
}

} // namespace cpp
} // namespace ply
