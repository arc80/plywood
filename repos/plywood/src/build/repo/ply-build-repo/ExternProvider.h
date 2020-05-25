/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-build-common/Core.h>

namespace ply {
namespace build {

struct DependencySource;
struct ExternProviderArgs;
struct Repo;
struct HostTools;
struct ToolchainInfo;
struct ProjectInstantiator;
struct Dependency;
struct ExternFolder;
struct RepoRegistry;
struct ExternFolderRegistry;

enum class ExternCommand {
    Status,
    Install,
    Instantiate,
};

struct ExternResult {
    enum Code {
        Unknown,
        BadArgs,
        UnsupportedHost,
        MissingPackageManager,
        UnsupportedToolchain,
        SupportedButNotInstalled,
        Installed,
        InstallFailed,
        Instantiated,
    };
    Code code = Unknown;
    String details;

    PLY_INLINE ExternResult(Code code, StringView details) : code{code}, details{details} {
    }
    PLY_INLINE bool isSupported() {
        return (this->code == SupportedButNotInstalled || this->code == Installed);
    }
};

struct ExternProvider {
    typedef ExternResult ExternFunc(ExternCommand cmd, ExternProviderArgs* args);

    const DependencySource* extern_ = nullptr;
    String providerName;
    const Repo* repo = nullptr;
    ExternFunc* externFunc = nullptr;

    PLY_BUILD_ENTRY String getFullyQualifiedName() const;
};

struct ExternProviderArgs {
    const ToolchainInfo* toolchain = nullptr;
    const ExternProvider* provider = nullptr;
    String providerArgs;

    // Only valid for Instantiate command:
    ProjectInstantiator* projInst = nullptr;
    Dependency* dep = nullptr;

    PLY_BUILD_ENTRY Tuple<ExternResult, ExternFolder*>
    findExistingExternFolder(StringView folderArgs) const;
    PLY_BUILD_ENTRY ExternFolder* createExternFolder(StringView folderArgs) const;
};

} // namespace build
} // namespace ply
