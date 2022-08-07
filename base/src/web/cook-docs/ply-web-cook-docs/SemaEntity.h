/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
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
            String srcPath; // FIXME: Deduplicate this (optimization)
            u32 lineNumber = 0;
        };

        Array<Title> titles;
        String markdownDesc;
        s32 categoryIndex = -1;
    };

    SemaEntity* class_ = nullptr;
    String classMarkdownDesc;
    Array<Entry> entries;
    Array<Category> categories;
};

// Note: SemaEntity doesn't really have to be refcounted.
// We could instead make them all directly owned by CookResult_ExtractAPI objects, and implement
// some kind of mark-and-sweep algorithm to clean up orphaned SemaEntities (eg. ones that are no
// longer accessible from any CookResult_ExtractAPI).
// But for now, stick with refcounting.
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
        static PLY_INLINE StringView getIndex(SemaEntity* ent) {
            return ent->name;
        }
        static PLY_INLINE bool less(const Index& a, const Index& b) {
            return a < b;
        }
        static PLY_INLINE void onItemMoved(SemaEntity*, void*) {
        }
    };

    PLY_REFLECT()
    // ply reflect off

    SemaEntity* parent = nullptr; // adds a reference if parent is a Namespace
    Type type = Namespace;
    String name;
    u128 hash = 0;                                  // If Class
    Reference<SemaEntity> templateParams = nullptr; // If template Class or Member
    Array<cpp::sema::QualifiedID> baseClasses;      // If Class
    Array<Reference<SemaEntity>> childSeq;          // If Class or ParamList
    BTree<ChildBTreeTraits> nameToChild;            // If Namespace, Class or ParamList
    cpp::sema::SingleDeclaration singleDecl;        // If Member

    // Specific to documentation extractor
    // Currently only set for Classes
    // (Perhaps this should be stored in a generic way so that the SemaEntity class can be reused)
    Owned<DocInfo> docInfo;

    PLY_INLINE void setParent(SemaEntity* p) {
        PLY_ASSERT(p);
        this->parent = p;
        if (p->type == Namespace) {
            p->incRef();
        }
    }
    ~SemaEntity();
    PLY_INLINE void onRefCountZero() {
        delete this;
    }
    PLY_INLINE bool isFunction() const {
        PLY_ASSERT(this->type == Type::Member);
        return this->singleDecl.dcor.prod && this->singleDecl.dcor.prod->function();
    }
    void setClassHash();
    void appendToQualifiedID(OutStream* outs) const;
    PLY_INLINE String getQualifiedID() const {
        PLY_ASSERT(this->type == SemaEntity::Class);
        MemOutStream mout;
        this->appendToQualifiedID(&mout);
        return mout.moveToString();
    }
    SemaEntity* lookup(StringView name, bool checkParents = true);
    SemaEntity* lookupChain(ArrayView<const StringView> components);
};

void dumpSemaEnts(OutStream* outs, SemaEntity* scope, u32 level = 0);

} // namespace docs
} // namespace ply
