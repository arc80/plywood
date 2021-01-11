/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-build-common/Core.h>
#include <ply-build-repo/TargetInstantiator.h>
#include <ply-build-repo/ExternProvider.h>

namespace ply {
namespace build {

struct Repo {
    struct TargetInstantiatorsTraits {
        using Key = StringView;
        using Item = Owned<TargetInstantiator>;
        static PLY_INLINE bool match(const Item& item, Key key) {
            return item->name == key;
        }
    };

    String repoName;
    HashMap<TargetInstantiatorsTraits> targetInstantiators;
    Array<Owned<DependencySource>> externs;
    Array<Owned<ExternProvider>> externProviders;
    Array<Repo*> childRepos;

    PLY_BUILD_ENTRY void addTargetInstantiator(Owned<TargetInstantiator>&& targetInst);
    PLY_BUILD_ENTRY void addExternProvider(StringView externName, StringView providerName,
                                           ExternProvider::ExternFunc* externFunc);

    const Repo* findChildRepo(StringView childName) const;
    PLY_INLINE Repo* findChildRepo(StringView childName) {
        return const_cast<Repo*>(const_cast<const Repo*>(this)->findChildRepo(childName));
    }
    const DependencySource* findExternImm(StringView externName) const;
    const DependencySource* findExternRecursive(StringView externName) const;
    const DependencySource* findOrCreateExtern(ArrayView<StringView>& comps, bool allowCreate);
    PLY_INLINE const DependencySource* findExtern(ArrayView<StringView>& comps) const {
        return const_cast<Repo*>(this)->findOrCreateExtern(comps, false);
    }
    const TargetInstantiator* findTargetInstantiatorImm(StringView targetInstName) const;
    const TargetInstantiator* findTargetInstantiatorRecursive(StringView targetInstName) const;
};

} // namespace build
} // namespace ply
