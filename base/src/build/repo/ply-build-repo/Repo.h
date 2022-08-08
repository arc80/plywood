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

    PLY_BUILD_ENTRY void addTargetInstantiator(Owned<TargetInstantiator>&& targetInst);
    PLY_BUILD_ENTRY void addExternProvider(StringView externName, StringView providerName,
                                           ExternProvider::ExternFunc* externFunc);

    const DependencySource* findExtern(StringView externName) const;
    const DependencySource* findOrCreateExtern(StringView externName, bool allowCreate);
    const TargetInstantiator* findTargetInstantiator(StringView targetInstName) const;
};

} // namespace build
} // namespace ply
