/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-web-cook-docs/Core.h>
#include <ply-web-cook-docs/Sema.h>
#include <ply-web-cook-docs/SymbolTitle.h>
#include <ply-runtime/container/BTree.h>
#include <ply-runtime/container/Hash128.h>

namespace ply {
namespace docs {

struct SemaEntity;

struct DocInfo {
    struct Category {
        String desc;
    };

    struct Entry {
        struct Title {
            SemaEntity* member = nullptr;
            String src_path; // FIXME: Deduplicate this (optimization)
            u32 line_number = 0;
        };

        Array<Title> titles;
        String markdown_desc;
        s32 category_index = -1;
    };

    SemaEntity* class_ = nullptr;
    String class_markdown_desc;
    Array<Entry> entries;
    Array<Category> categories;
};

// Note: SemaEntity doesn't really have to be refcounted.
// We could instead make them all directly owned by CookResult_ExtractAPI objects, and
// implement some kind of mark-and-sweep algorithm to clean up orphaned SemaEntities
// (eg. ones that are no longer accessible from any CookResult_ExtractAPI). But for now,
// stick with refcounting.
struct SemaEntity : RefCounted<SemaEntity> {
    enum Type {
        Namespace, // the global namespace has no parent
        Class,
        Member, // can also be a global variable or function
        TemplateParamList,
        TemplateParam,
        FunctionParam
    };

    struct ChildBTreeTraits {
        using Item = SemaEntity*;
        using Index = StringView;
        static constexpr u32 NodeCapacity = 8;
        static StringView get_index(SemaEntity* ent) {
            return ent->name;
        }
        static bool less(const Index& a, const Index& b) {
            return a < b;
        }
        static void on_item_moved(SemaEntity*, void*) {
        }
    };

    PLY_REFLECT()
    // ply reflect off

    SemaEntity* parent = nullptr; // adds a reference if parent is a Namespace
    Type type = Namespace;
    String name;
    u128 hash = 0;                                   // If Class
    Reference<SemaEntity> template_params = nullptr; // If template Class or Member
    Array<cpp::sema::QualifiedID> base_classes;      // If Class
    Array<Reference<SemaEntity>> child_seq;          // If Class or ParamList
    BTree<ChildBTreeTraits> name_to_child;           // If Namespace, Class or ParamList
    cpp::sema::SingleDeclaration single_decl;        // If Member

    // Specific to documentation extractor
    // Currently only set for Classes
    // (Perhaps this should be stored in a generic way so that the SemaEntity class can
    // be reused)
    Owned<DocInfo> doc_info;

    void set_parent(SemaEntity* p) {
        PLY_ASSERT(p);
        this->parent = p;
        if (p->type == Namespace) {
            p->inc_ref();
        }
    }
    ~SemaEntity();
    void on_ref_count_zero() {
        delete this;
    }
    bool is_function() const {
        PLY_ASSERT(this->type == Type::Member);
        return this->single_decl.dcor.prod && this->single_decl.dcor.prod->function();
    }
    void set_class_hash();
    void append_to_qualified_id(OutStream* outs) const;
    String get_qualified_id() const {
        PLY_ASSERT(this->type == SemaEntity::Class);
        MemOutStream mout;
        this->append_to_qualified_id(&mout);
        return mout.move_to_string();
    }
    SemaEntity* lookup(StringView name, bool check_parents = true);
    SemaEntity* lookup_chain(ArrayView<const StringView> components);
};

void dump_sema_ents(OutStream* outs, SemaEntity* scope, u32 level = 0);

} // namespace docs
} // namespace ply
