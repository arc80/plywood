/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-build-common/Core.h>
#include <ply-build-repo/Repo.h>
#include <ply-build-target/CMakeLists.h>

namespace ply {
namespace build {

struct DependencySource;

struct RepoRegistry {
    struct RepoMapTraits {
        using Key = StringView;
        using Item = Owned<Repo>;
        static PLY_INLINE Key comparand(const Item& item) {
            return item->repoName;
        }
    };

    u128 moduleDefSignature = 0;
    HashMap<RepoMapTraits> repos;

    static Owned<RepoRegistry> instance_;
    static Owned<RepoRegistry> create();
    static PLY_INLINE const RepoRegistry* get() {
        PLY_ASSERT(instance_);
        return instance_;
    }

    PLY_NO_INLINE const TargetInstantiator* findTargetInstantiator(StringView targetName) const;
    PLY_NO_INLINE const DependencySource* findExtern(StringView externName) const;
    PLY_NO_INLINE const ExternProvider* getExternProvider(StringView qualifiedName) const;
    PLY_NO_INLINE String getShortDepSourceName(const DependencySource* depSrc) const;
    PLY_NO_INLINE String getShortProviderName(const ExternProvider* externProvider) const;
};

u128 parseSignatureString(StringView str);
String signatureToString(u128 sig);

} // namespace build
} // namespace ply
