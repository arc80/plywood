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
        sig.hi = StringViewReader{str.left(16)}.parse<u64>(fmt::Radix{16});
        sig.lo = StringViewReader{str.subStr(16)}.parse<u64>(fmt::Radix{16});
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
        auto repoCursor = this->repoReg->repos.insertOrFind(idll.repoName);
        if (repoCursor.wasFound()) {
            return *repoCursor; // already instantiated
        }
        // FIXME: Detect circular dependencies and report them gracefully
        Repo* repo = new Repo;
        repo->repoName = idll.repoName;
        *repoCursor = repo;

        // Load info.pylon and fetch children recursively
        String infoPath =
            NativePath::join(PLY_WORKSPACE_FOLDER, "repos", idll.repoName, "info.pylon");
        String infoText = FileSystem::native()->loadTextAutodetect(infoPath).first;
        FSResult result = FileSystem::native()->lastResult();
        if (result == FSResult::OK) {
            pylon::Parser parser;
            parser.setErrorCallback([&](const pylon::ParseError& err) {
                StringWriter sw;
                sw << infoPath;
                parser.dumpError(err, sw);
                ErrorHandler::log(ErrorHandler::Error, sw.moveToString());
            });
            pylon::Parser::Result parseResult = parser.parse(infoText);
            if (parser.anyError()) {
                return nullptr;
            }

            // FIXME: Use reflection
            // importInto(TypedPtr::bind(this), aRoot);
            for (const pylon::Node* dependency : parseResult.root->get("dependsOn")->arrayView()) {
                StringView depRepoName = dependency->text();
                s32 j = find(this->idlls.dlls.view(),
                             [&](const InstantiatedDLL& d) { return d.repoName == depRepoName; });
                if (j < 0) {
                    FileLocation loc = parseResult.fileLocMap.getFileLocation(dependency->fileOfs);
                    ErrorHandler::log(
                        ErrorHandler::Fatal,
                        String::format("{}({}, {}): error: Can't find repo named '{}'\n", infoPath,
                                       loc.lineNumber, loc.columnNumber, depRepoName));
                    return nullptr; // shouldn't get here
                }
                Repo* childRepo = this->instantiate(this->idlls.dlls[j]);
                if (!childRepo) {
                    return nullptr;
                }
                if (findItem(repo->childRepos.view(), childRepo) >= 0) {
                    FileLocation loc = parseResult.fileLocMap.getFileLocation(dependency->fileOfs);
                    ErrorHandler::log(
                        ErrorHandler::Fatal,
                        String::format("{}({}, {}): error: Duplicate child repo '{}'\n", infoPath,
                                       loc.lineNumber, loc.columnNumber, depRepoName));
                    return nullptr; // shouldn't get here
                }
                repo->childRepos.append(childRepo);
            }
        } else if (result != FSResult::NotFound) {
            ErrorHandler::log(ErrorHandler::Fatal,
                              String::format("Error loading '{}'\n", infoPath));
            return nullptr; // shouldn't get here
        }

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
    Array<StringView> comps = splitName(targetName, "module name");
    if (!comps)
        return nullptr;

    auto repoCursor = this->repos.find(comps[0]);
    if (repoCursor.wasFound()) {
        // First component is a repo name
        if (comps.numItems() == 1) {
            ErrorHandler::log(
                ErrorHandler::Error,
                String::format("'{}' is a repo; expected a module name\n", targetName));
            return nullptr;
        } else if (comps.numItems() == 2) {
            const TargetInstantiator* targetInst =
                (*repoCursor)->findTargetInstantiatorImm(comps[1]);
            if (targetInst) {
                return targetInst;
            }
            ErrorHandler::log(
                ErrorHandler::Error,
                String::format("Can't find module '{}' in repo '{}'\n", comps[1], comps[0]));
            return nullptr;
        }
        ErrorHandler::log(ErrorHandler::Error,
                          String::format("Too many components in module name '{}'\n", targetName));
        return nullptr;
    }

    if (comps.numItems() != 1) {
        ErrorHandler::log(ErrorHandler::Error,
                          String::format("Too many components in module name '{}'\n", targetName));
        return nullptr;
    }
    const TargetInstantiator* targetInst = nullptr;
    bool conflict = false;
    for (Repo* repo : this->repos) {
        if (const TargetInstantiator* t = repo->findTargetInstantiatorImm(comps[0])) {
            if (targetInst) {
                conflict = true;
            } else {
                targetInst = t;
            }
        }
    }
    if (!targetInst) {
        ErrorHandler::log(ErrorHandler::Error,
                          String::format("Can't find module '{}' in any repo\n", targetName));
        return nullptr;
    } else if (conflict) {
        ErrorHandler::log(
            ErrorHandler::Error,
            String::format(
                "Module '{}' was found in multiple repos; use a repo prefix such as '{}'\n",
                targetName, targetInst->getFullyQualifiedName()));
        return nullptr;
    }

    return targetInst; // Success!
}

PLY_NO_INLINE const DependencySource* findExternInternal(const RepoRegistry* repoReg,
                                                         ArrayView<const StringView>& comps) {
    PLY_ASSERT(!comps.isEmpty());
    auto repoCursor = repoReg->repos.find(comps[0]);
    if (repoCursor.wasFound()) {
        comps.offsetHead(1);
        if (comps.isEmpty()) {
            ErrorHandler::log(ErrorHandler::Error,
                              String::format("'{}' is a repo; expected a module name\n",
                                             (*repoCursor)->repoName));
            return nullptr;
        }
        StringView externName = comps[0];
        comps.offsetHead(1);
        const DependencySource* extern_ = (*repoCursor)->findExternImm(externName);
        if (!extern_) {
            ErrorHandler::log(ErrorHandler::Error,
                              String::format("Can't find extern '{}' in repo '{}'\n", externName[1],
                                             (*repoCursor)->repoName));
            return nullptr;
        }
        return extern_;
    }

    const DependencySource* extern_ = nullptr;
    bool conflict = false;
    StringView externName = comps[0];
    comps.offsetHead(1);
    for (Repo* repo : repoReg->repos) {
        if (const DependencySource* d = repo->findExternImm(externName)) {
            if (extern_) {
                conflict = true;
            } else {
                extern_ = d;
            }
        }
    }

    if (!extern_) {
        ErrorHandler::log(ErrorHandler::Error,
                          String::format("Can't find extern '{}' in any repo\n", externName));
        return nullptr;
    } else if (conflict) {
        ErrorHandler::log(
            ErrorHandler::Error,
            String::format(
                "Extern '{}' was found in multiple repos; use a repo prefix such as '{}'\n",
                externName, extern_->getFullyQualifiedName()));
        return nullptr;
    }

    return extern_; // Success
}

PLY_NO_INLINE const DependencySource* RepoRegistry::findExtern(StringView externName) const {
    Array<StringView> compsOwner = splitName(externName, "extern name");
    if (!compsOwner)
        return nullptr;
    ArrayView<const StringView> comps = compsOwner.view();
    const DependencySource* extern_ = findExternInternal(this, comps);
    if (!extern_)
        return nullptr;
    if (!comps.isEmpty()) {
        ErrorHandler::log(ErrorHandler::Error,
                          String::format("Too many components in extern name '{}'\n", externName));
    }
    return extern_;
}

PLY_NO_INLINE const ExternProvider*
RepoRegistry::getExternProvider(StringView qualifiedName) const {
    Array<StringView> compsOwner = splitName(qualifiedName, "extern provider name");
    if (!compsOwner)
        return nullptr;
    ArrayView<const StringView> comps = compsOwner.view();
    const DependencySource* extern_ = findExternInternal(this, comps);
    if (!extern_)
        return nullptr;

    // Find provider
    if (comps.isEmpty()) {
        ErrorHandler::log(
            ErrorHandler::Error,
            String::format("Not enough components in extern provider name '{}'\n", qualifiedName));
    }
    auto repoCursor = this->repos.find(comps[0]);
    if (repoCursor.wasFound()) {
        comps.offsetHead(1);
        if (comps.isEmpty()) {
            ErrorHandler::log(ErrorHandler::Error,
                              String::format("'{}' is a repo; expected an extern provider name\n",
                                             (*repoCursor)->repoName));
            return nullptr;
        }
        s32 j = find((*repoCursor)->externProviders.view(), [&](const ExternProvider* p) {
            return p->extern_ == extern_ && p->providerName == comps[0];
        });
        if (j < 0) {
            ErrorHandler::log(ErrorHandler::Error,
                              String::format("Can't find extern provider '{}' in repo '{}'\n",
                                             qualifiedName, (*repoCursor)->repoName));
            return nullptr;
        }
        comps.offsetHead(1);
        if (!comps.isEmpty()) {
            ErrorHandler::log(ErrorHandler::Error,
                              String::format("Too many components in extern provider name '{}'\n",
                                             qualifiedName));
            return nullptr;
        }
        return (*repoCursor)->externProviders[j];
    } else {
        if (comps.isEmpty()) {
            ErrorHandler::log(
                ErrorHandler::Error,
                String::format("'{}' is an extern; expected an extern provider name\n",
                               qualifiedName));
            return nullptr;
        }
        const ExternProvider* provider = nullptr;
        bool conflict = false;
        for (const Repo* repo : this->repos) {
            s32 j = find(repo->externProviders.view(), [&](const ExternProvider* p) {
                return p->extern_ == extern_ && p->providerName == comps[0];
            });
            if (j >= 0) {
                if (provider) {
                    conflict = true;
                } else {
                    provider = repo->externProviders[j];
                }
            }
        }

        if (!provider) {
            ErrorHandler::log(
                ErrorHandler::Error,
                String::format("Can't find extern provider '{}' for extern '{}' in any repo\n",
                               comps[0], this->getShortDepSourceName(extern_)));
            return nullptr;
        } else if (conflict) {
            ErrorHandler::log(ErrorHandler::Error,
                              String::format("Extern provider '{}' for extern '{}'  was found in "
                                             "multiple repos; use a repo prefix such as '{}'\n",
                                             comps[0], this->getShortDepSourceName(extern_),
                                             provider->getFullyQualifiedName()));
            return nullptr;
        }

        comps.offsetHead(1);
        if (!comps.isEmpty()) {
            ErrorHandler::log(ErrorHandler::Error,
                              String::format("Too many components in extern provider name '{}'\n",
                                             qualifiedName));
            return nullptr;
        }
        return provider;
    }
}

PLY_NO_INLINE String RepoRegistry::getShortDepSourceName(const DependencySource* depSrc) const {
    for (const Repo* repo : this->repos) {
        if (repo == depSrc->repo)
            continue;
        s32 index = find(repo->externs.view(),
                         [&](const DependencySource* d) { return d->name == depSrc->name; });
        if (index >= 0) {
            // Extern name is defined by multiple repos, so qualify it:
            return String::format("{}.{}", depSrc->repo->repoName, depSrc->name);
        }
    }
    return depSrc->name;
}

PLY_NO_INLINE String
RepoRegistry::getShortProviderName(const ExternProvider* externProvider) const {
    String shortExternName = this->getShortDepSourceName(externProvider->extern_);
    for (const Repo* repo : this->repos) {
        if (repo == externProvider->repo)
            continue;
        s32 index = find(repo->externProviders.view(), [&](const ExternProvider* p) {
            return p->extern_ == externProvider->extern_ &&
                   p->providerName == externProvider->providerName;
        });
        if (index >= 0) {
            // An extern provider with this name and for this extern was defined by multiple repos,
            // so qualify it:
            return String::format("{}.{}.{}", shortExternName, externProvider->repo->repoName,
                                  externProvider->providerName);
        }
    }
    return String::format("{}.{}", shortExternName, externProvider->providerName);
}

} // namespace build
} // namespace ply
