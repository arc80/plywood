/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-web-cook-docs/Core.h>
#include <ply-web-cook-docs/SemaEntity.h>
#include <ply-web-cook-docs/SemaToString.h>
#include <ply-web-cook-docs/WebCookerIndex.h>
#include <ply-cook/CookJob.h>

namespace ply {
namespace docs {

SemaEntity::~SemaEntity() {
    if (this->parent && this->parent->type == Namespace) {
        // Remove from parent's name_to_child map
        auto iter =
            this->parent->name_to_child.find_first_greater_or_equal_to(this->name);
        for (; iter.is_valid() && iter.get_item()->name == this->name; iter.next()) {
            if (iter.get_item() == this) {
                this->parent->name_to_child.remove(iter);
                goto found_in_parent;
            }
        }
        PLY_ASSERT(0); // not found
    found_in_parent:
        this->parent->dec_ref();
    }
}

void SemaEntity::set_class_hash() {
    PLY_ASSERT(this->type == SemaEntity::Class);
    PLY_ASSERT(this->hash == 0);

    Hash128 hasher;
    hasher.append(this->get_qualified_id());
    // FIXME
    /*
    for (const SemaEntity* member : this->child_seq) {
        if (member->type == SemaEntity::Member && !member->doc_string.is_empty()) {
            char null_str[1] = {0};
            hasher.append({null_str, 1});
            hasher.append(cpp::sema::to_string(member->single_decl).buffer_view());
            hasher.append({null_str, 1});
            member->doc_string.append_to(hasher);
        }
    }
    */
    this->hash = hasher.get();
}

void SemaEntity::append_to_qualified_id(OutStream* outs) const {
    PLY_ASSERT(this->parent); // Must not be the global namespace
    if (this->parent->parent) {
        this->parent->append_to_qualified_id(outs);
        *outs << "::";
    }
    *outs << this->name;
}

SemaEntity* SemaEntity::lookup(StringView name, bool check_parents) {
    auto iter = this->name_to_child.find_first_greater_or_equal_to(name);
    if (iter.is_valid() && iter.get_item()->name == name) {
        return iter.get_item();
    }
    if (check_parents && this->parent) {
        return this->parent->lookup(name);
    }
    return nullptr;
}

SemaEntity* SemaEntity::lookup_chain(ArrayView<const StringView> components) {
    if (!components)
        return nullptr;
    bool first = true;
    SemaEntity* result = this;
    for (u32 i = 0; i < components.num_items; i++) {
        // Qualified IDs should only work on namespaces and classes
        if (!first && (result->type != SemaEntity::Namespace) &&
            (result->type != SemaEntity::Class))
            return nullptr;
        result = result->lookup(components[i],
                                first); // Only check parents on first component
        if (!result)
            return nullptr;
        first = false;
    }
    return result;
}

// FIXME: Support BTree const iteration and make scope const
void dump_sema_ents(OutStream* outs, SemaEntity* scope, u32 level) {
    if (scope->type == SemaEntity::Member) {
        outs->format("{}{}\n", StringView{"  "} * level, to_string(scope->single_decl));
    } else {
        outs->format("{}{}\n", StringView{"  "} * level, scope->name);
        for (SemaEntity* child_ent : scope->child_seq) {
            dump_sema_ents(outs, child_ent, level + 1);
        }
    }
}

} // namespace docs
} // namespace ply

#include "codegen/SemaEntity.inl" //%%
