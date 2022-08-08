/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-common/Core.h>
#include <ply-build-repo/RepoRegistry.h>
#include <ply-build-repo/BuildInstantiatorDLLs.h>
#include <pylon/Parse.h>
#include <ply-build-repo/ExternProvider.h>
#include <ply-runtime/algorithm/Find.h>
#include <ply-runtime/string/WString.h>
#include <ply-build-repo/ErrorHandler.h>

#if PLY_TARGET_POSIX
#include <dlfcn.h>
#endif

namespace ply {
namespace build {

Owned<RepoRegistry> RepoRegistry::instance_;

u128 parseSignatureString(StringView str) {
    if (str.numBytes != 32) {
        return 0;
    } else {
        u128 sig = 0;
        sig.hi = ViewInStream{str.left(16)}.parse<u64>(fmt::Radix{16});
        sig.lo = ViewInStream{str.subStr(16)}.parse<u64>(fmt::Radix{16});
        return sig;
    }
}

String signatureToString(u128 sig) {
    return (StringView{"0"} * 16 + String::from(fmt::Hex{sig.hi})).right(16) +
           (StringView{"0"} * 16 + String::from(fmt::Hex{sig.lo})).right(16);
}

struct RepoInstantiator {
    InstantiatedDLLs idlls;
    RepoRegistry* repoReg;

    Repo* instantiate(const InstantiatedDLL& idll) {
        Repo* repo = &this->repoReg->repo;
        PLY_ASSERT(!repo->repoName);
        repo->repoName = idll.repoName;

        // Load DLL
        typedef void RegFunc(Repo*);
#if PLY_TARGET_POSIX
        void* module = dlopen(idll.dllPath.withNullTerminator().bytes, RTLD_LAZY);
        if (!module) {
            const char* errStr = dlerror();
            ErrorHandler::log(ErrorHandler::Fatal,
                              String::format("Error loading '{}':\n{}\n", idll.dllPath, errStr));
            return nullptr; // shouldn't get here
        }
        RegFunc* registerInstantiators = (RegFunc*) dlsym(module, "registerInstantiators");
        if (!registerInstantiators) {
            ErrorHandler::log(
                ErrorHandler::Fatal,
                String::format("Error: Can't find entry point in '{}'\n", idll.dllPath));
            // If the error wasn't fatal, we would have to unload the module here
            return nullptr; // shouldn't get here
        }
#elif PLY_TARGET_WIN32
        // FIXME: Convert to UTF-16
        HMODULE module = LoadLibraryW(win32PathArg(idll.dllPath));
        if (!module) {
            ErrorHandler::log(ErrorHandler::Fatal,
                              String::format("Error loading '{}'\n", idll.dllPath));
            return nullptr; // shouldn't get here
        }
        RegFunc* registerInstantiators = (RegFunc*) GetProcAddress(module, "registerInstantiators");
        if (!registerInstantiators) {
            ErrorHandler::log(
                ErrorHandler::Fatal,
                String::format("Error: Can't find entry point in '{}'\n", idll.dllPath));
            // If the error wasn't fatal, we would have to unload the module here
            return nullptr; // shouldn't get here
        }
#else
#error "Unsupported platform"
#endif

        // Register instantiators in DLL
        registerInstantiators(repo);

        // FIXME: Unload DLL here?
        return repo;
    }
};

PLY_NO_INLINE Owned<RepoRegistry> RepoRegistry::create() {
    PLY_ASSERT(!instance_);
    Owned<RepoRegistry> repoReg = new RepoRegistry;

    RepoInstantiator inst;
    inst.repoReg = repoReg;
    inst.idlls = buildInstantiatorDLLs(false);
    for (const InstantiatedDLL& idll : inst.idlls.dlls) {
        inst.instantiate(idll);
    }

    repoReg->moduleDefSignature = inst.idlls.signature;
    return repoReg;
}

PLY_NO_INLINE const TargetInstantiator*
RepoRegistry::findTargetInstantiator(StringView targetName) const {
    const TargetInstantiator* targetInst = this->repo.findTargetInstantiator(targetName);
    if (!targetInst) {
        ErrorHandler::log(ErrorHandler::Error,
                          String::format("Can't find module '{}'\n", targetName));
        return nullptr;
    }
    return targetInst;
}

PLY_NO_INLINE const DependencySource* RepoRegistry::findExtern(StringView externName) const {
    const DependencySource* extern_ = this->repo.findExtern(externName);
    if (!extern_) {
        ErrorHandler::log(ErrorHandler::Error,
                          String::format("Can't find extern '{}'\n", externName));
        return nullptr;
    }

    return extern_; // Success
}

PLY_NO_INLINE const ExternProvider*
RepoRegistry::getExternProvider(StringView qualifiedName) const {
    Array<StringView> comps = qualifiedName.splitByte('.');
    if (comps.numItems() != 2) {
        ErrorHandler::log(ErrorHandler::Error,
                          "Extern provider name must have exactly 2 components\n");
        return nullptr;
    }

    StringView externName = comps[0];
    const DependencySource* extern_ = this->repo.findExtern(externName);
    if (!extern_) {
        ErrorHandler::log(ErrorHandler::Error,
                          String::format("Can't find extern '{}'\n", externName));
        return nullptr;
    }

    // Find provider
    StringView providerName = comps[1];
    const ExternProvider* provider = nullptr;
    {
        s32 j = find(this->repo.externProviders, [&](const ExternProvider* p) {
            return p->extern_ == extern_ && p->providerName == providerName;
        });
        if (j >= 0) {
            provider = this->repo.externProviders[j];
        }
    }

    if (!provider) {
        ErrorHandler::log(ErrorHandler::Error,
                          String::format("Can't find extern provider '{}' for extern '{}'\n",
                                         providerName, externName));
        return nullptr;
    }

    return provider;
}

} // namespace build
} // namespace ply
