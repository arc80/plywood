/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-web-cook-docs/Core.h>
#include <ply-web-cook-docs/SemaEntity.h>
#include <ply-web-cook-docs/SemaToString.h>
#include <ply-web-cook-docs/WebCookerIndex.h>
#include <ply-cook/CookJob.h>

namespace ply {
namespace docs {

SemaEntity::~SemaEntity() {
    if (this->parent && this->parent->type == Namespace) {
        // Remove from parent's nameToChild map
        auto iter = this->parent->nameToChild.findFirstGreaterOrEqualTo(this->name);
        for (; iter.isValid() && iter.getItem()->name == this->name; iter.next()) {
            if (iter.getItem() == this) {
                this->parent->nameToChild.remove(iter);
                goto foundInParent;
            }
        }
        PLY_ASSERT(0); // not found
    foundInParent:
        this->parent->decRef();
    }
}

void SemaEntity::setClassHash() {
    PLY_ASSERT(this->type == SemaEntity::Class);
    PLY_ASSERT(this->hash == 0);

    Hash128 hasher;
    hasher.append(this->getQualifiedID());
    // FIXME
    /*
    for (const SemaEntity* member : this->childSeq) {
        if (member->type == SemaEntity::Member && !member->docString.isEmpty()) {
            char nullStr[1] = {0};
            hasher.append({nullStr, 1});
            hasher.append(cpp::sema::toString(member->singleDecl).bufferView());
            hasher.append({nullStr, 1});
            member->docString.appendTo(hasher);
        }
    }
    */
    this->hash = hasher.get();
}

void SemaEntity::appendToQualifiedID(OutStream* outs) const {
    PLY_ASSERT(this->parent); // Must not be the global namespace
    if (this->parent->parent) {
        this->parent->appendToQualifiedID(outs);
        *outs << "::";
    }
    *outs << this->name;
}

SemaEntity* SemaEntity::lookup(StringView name, bool checkParents) {
    auto iter = this->nameToChild.findFirstGreaterOrEqualTo(name);
    if (iter.isValid() && iter.getItem()->name == name) {
        return iter.getItem();
    }
    if (checkParents && this->parent) {
        return this->parent->lookup(name);
    }
    return nullptr;
}

SemaEntity* SemaEntity::lookupChain(ArrayView<const StringView> components) {
    if (!components)
        return nullptr;
    bool first = true;
    SemaEntity* result = this;
    for (u32 i = 0; i < components.numItems; i++) {
        // Qualified IDs should only work on namespaces and classes
        if (!first && (result->type != SemaEntity::Namespace) && (result->type != SemaEntity::Class))
            return nullptr;
        result = result->lookup(components[i], first); // Only check parents on first component
        if (!result)
            return nullptr;
        first = false;
    }
    return result;
}

// FIXME: Support BTree const iteration and make scope const
void dumpSemaEnts(OutStream* outs, SemaEntity* scope, u32 level) {
    if (scope->type == SemaEntity::Member) {
        outs->format("{}{}\n", StringView{"  "} * level, toString(scope->singleDecl));
    } else {
        outs->format("{}{}\n", StringView{"  "} * level, scope->name);
        for (SemaEntity* childEnt : scope->childSeq) {
            dumpSemaEnts(outs, childEnt, level + 1);
        }
    }
}

} // namespace docs
} // namespace ply

#include "codegen/SemaEntity.inl" //%%
