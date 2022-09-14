/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-common/Core.h>
#include <ply-build-folder/BuildFolder.h>
#include <pylon/Parse.h>
#include <pylon/Write.h>
#include <pylon-reflect/Import.h>
#include <pylon-reflect/Export.h>
#include <ply-build-repo/RepoRegistry.h>
#include <ply-build-repo/ProjectInstantiator.h>
#include <ply-build-repo/ExternProvider.h>
#include <ply-build-repo/ErrorHandler.h>
#include <ply-build-repo/ProjectInstantiationEnv.h>
#include <ply-build-target/Dependency.h>
#include <ply-build-target/CMakeLists.h>
#include <ply-runtime/algorithm/Find.h>
#include <ply-build-provider/ExternFolderRegistry.h>
#include <ply-build-provider/HostTools.h>
#include <ply-runtime/container/Hash128.h>
#include <ply-build-repository/Instantiate.h>

namespace ply {
namespace build {

struct BuildFolderName {
    static PLY_NO_INLINE String getFullPath(StringView buildFolderName) {
        if (buildFolderName.isEmpty()) {
            return NativePath::join(PLY_WORKSPACE_FOLDER, "data/dummy_build_folder/");
        } else {
            return NativePath::join(PLY_WORKSPACE_FOLDER, "data/build", buildFolderName, "");
        }
    }

    static PLY_NO_INLINE String getInfoPath(StringView buildFolderName) {
        PLY_ASSERT(!buildFolderName.isEmpty());
        return NativePath::join(getFullPath(buildFolderName), "info.pylon");
    }
};

String BuildFolder::getAbsPath() const {
    return BuildFolderName::getFullPath(this->buildFolderName);
}

PLY_NO_INLINE Owned<BuildFolder> BuildFolder::create(StringView buildFolderName,
                                                     StringView solutionName) {
    BuildFolder* bf = new BuildFolder;
    bf->buildFolderName = buildFolderName;
    bf->buildSystemSignature = pylon::Node::createObject();
    bf->solutionName = solutionName;
    // Don't save it yet
    return bf;
}

PLY_NO_INLINE Owned<BuildFolder> BuildFolder::autocreate(StringView baseName) {
    String buildFolderName = baseName;
    for (u32 prefix = 0; prefix < 1000; prefix++) {
        if (FileSystem::native()->exists(NativePath::join(
                PLY_WORKSPACE_FOLDER, "data/build", buildFolderName)) == ExistsResult::NotFound) {
            return BuildFolder::create(buildFolderName, buildFolderName);
        }
        prefix++;
        buildFolderName = baseName + (StringView{"00"} + String::from(prefix)).right(3);
    }
    ErrorHandler::log(
        ErrorHandler::Fatal,
        String::format("Unable to generate a unique build folder name for '{}'", baseName));
    return nullptr;
}

PLY_NO_INLINE Owned<BuildFolder> BuildFolder::load(StringView buildFolderName) {
    String infoPath = BuildFolderName::getInfoPath(buildFolderName);
    String strContents = FileSystem::native()->loadTextAutodetect(infoPath).first;
    if (FileSystem::native()->lastResult() != FSResult::OK)
        return nullptr;

    Owned<pylon::Node> aRoot = pylon::Parser{}.parse(infoPath, strContents).root;
    if (!aRoot->isValid())
        return nullptr;

    Owned<BuildFolder> info = pylon::import<BuildFolder>(aRoot);
    info->buildFolderName = buildFolderName;
    info->buildSystemSignature = aRoot->remove("buildSystemSignature");
    if (!info->buildSystemSignature) {
        info->buildSystemSignature = pylon::Node::createObject();
    }

    // Fixup files saved by older Plytool versions:
    for (String& rootTarget : info->rootTargets) {
        rootTarget = String{rootTarget.splitByte('.').back()};
    }
    for (String& makeShared : info->makeShared) {
        makeShared = String{makeShared.splitByte('.').back()};
    }
    for (String& externSelector : info->externSelectors) {
        Array<StringView> comps = externSelector.splitByte('.');
        u32 keep = max(comps.numItems(), 2u);
        externSelector = StringView{"."}.join(comps.subView(comps.numItems() - keep));
    }
    info->activeTarget = String{info->activeTarget.splitByte('.').back()};

    return info;
}

PLY_NO_INLINE bool BuildFolder::save() const {
    Owned<pylon::Node> aRoot = pylon::exportObj(AnyObject::bind(this));
    aRoot->set("buildSystemSignature", this->buildSystemSignature->copy());
    String strContents = pylon::toString(aRoot);
    String infoPath = BuildFolderName::getInfoPath(this->buildFolderName);
    FSResult rc = FileSystem::native()->makeDirsAndSaveTextIfDifferent(
        infoPath, strContents, TextFormat::platformPreference());
    return (rc == FSResult::OK || rc == FSResult::Unchanged);
}

Owned<pylon::Node> toolchainInfoFromCMakeOptions(const CMakeGeneratorOptions& cmakeOpts) {
    Owned<pylon::Node> toolchain = pylon::Node::createObject();
    toolchain->set("buildSystem", pylon::Node::createText(cmakeOpts.generator.view()));

    if (cmakeOpts.toolchainFile == "ios") {
        toolchain->set("targetPlatform", pylon::Node::createText("ios"));
        toolchain->set("apple", pylon::Node::createText("true"));
    } else if (!cmakeOpts.generator) {
        // Generator name is allowed to be blank when generating the bootstrap
    } else if (cmakeOpts.generator.startsWith("Visual Studio")) {
        toolchain->set("targetPlatform", pylon::Node::createText("windows"));
        toolchain->set("arch",
                       pylon::Node::createText(cmakeOpts.platform == "Win32" ? "x86" : "x64"));
    } else if (cmakeOpts.generator == "Xcode") {
        toolchain->set("targetPlatform", pylon::Node::createText("macos"));
        toolchain->set("apple", pylon::Node::createText("true"));
        toolchain->set("arch", pylon::Node::createText("x64"));
    } else if (cmakeOpts.generator == "Unix Makefiles") {
#if PLY_KERNEL_LINUX
        toolchain->set("targetPlatform", pylon::Node::createText("linux"));
#if PLY_CPU_X64
        toolchain->set("arch", pylon::Node::createText("x64"));
#elif PLY_CPU_X86
        toolchain->set("arch", pylon::Node::createText("x86"));
#else
        // FIXME: Handle more gracefully
        PLY_ASSERT(0); // Unsupported target platform
#endif
#elif PLY_TARGET_APPLE
        toolchain->set("targetPlatform", pylon::Node::createText("macos"));
        toolchain->set("apple", pylon::Node::createText("true"));
        toolchain->set("arch", pylon::Node::createText("x64"));
#else
        // FIXME: Handle more gracefully
        PLY_ASSERT(0); // Unsupported target system
#endif
    } else {
        // FIXME: Handle more gracefully
        PLY_ASSERT(0); // Unsupported generator
    }
    return toolchain;
}

Owned<ProjectInstantiationEnv> BuildFolder::createEnvironment() const {
    Owned<ProjectInstantiationEnv> env = new ProjectInstantiationEnv;
    env->cmakeOptions = &this->cmakeOptions;
    env->config = this->activeConfig.view();
    env->toolchain = toolchainInfoFromCMakeOptions(this->cmakeOptions);
    env->buildFolderPath = BuildFolderName::getFullPath(this->buildFolderName);
    for (StringView selName : this->externSelectors) {
        const ExternProvider* prov = RepoRegistry::get()->getExternProvider(selName);
        if (!prov) {
            ErrorHandler::log(
                ErrorHandler::Error,
                String::format("Invalid extern provider '{}' selected in build folder '{}'\n",
                               selName, this->buildFolderName));
            env->isValid = false;
            return env;
        }
        env->externSelectors.append(prov);
    }
    return env;
}

PLY_NO_INLINE ProjectInstantiationResult
BuildFolder::instantiateAllTargets(bool isGenerating) const {
    Owned<ProjectInstantiationEnv> env = this->createEnvironment();
    env->isGenerating = isGenerating;
    ProjectInstantiationResult instResult;
    ProjectInstantiator projInst;
    projInst.env = env;
    projInst.result = &instResult;

    for (StringView targetName : this->rootTargets) {
        const TargetInstantiator* targetInst =
            RepoRegistry::get()->findTargetInstantiator(targetName);
        if (!targetInst) {
            ErrorHandler::log(ErrorHandler::Error,
                              String::format("Invalid root target '{}' in build folder '{}'\n",
                                             targetName, this->buildFolderName));
            instResult.isValid = false;
            return instResult;
        }
        bool makeShared = find(this->makeShared, targetName) >= 0;
        projInst.instantiate(targetInst, makeShared);
    }

    projInst.propagateAllDependencies();
    return instResult;
}

PLY_NO_INLINE DependencyTree BuildFolder::buildDepTree() const {
    Owned<ProjectInstantiationEnv> env = this->createEnvironment();
    ProjectInstantiationResult instResult;
    ProjectInstantiator projInst;
    DependencyTree depTree;
    projInst.env = env;
    projInst.result = &instResult;
    projInst.depTreeNode = &depTree;

    for (StringView targetName : this->rootTargets) {
        const TargetInstantiator* targetInst =
            RepoRegistry::get()->findTargetInstantiator(targetName);
        /*
        FIXME: Report bad root targets somehow
        if (!targetInst) {
            ErrorHandler::log(ErrorHandler::Error,
                              String::format("Invalid root target '{}' in build folder '{}'\n",
                                             targetName, this->buildFolderName));
            instResult.isValid = false;
            return instResult;
        }
        */
        bool makeShared = find(this->makeShared, targetName) >= 0;
        projInst.instantiate(targetInst, makeShared);
    }
    return depTree;
}

PLY_NO_INLINE u128 BuildFolder::currentBuildSystemSignature() const {
    Hash128 h;
    u128 mds = RepoRegistry::get()->moduleDefSignature;
    h.append({(const char*) &mds, sizeof(mds)});
    h.append(this->solutionName);
    this->cmakeOptions.appendTo(h);
    for (StringView rt : this->rootTargets) {
        h.append(rt);
    }
    for (StringView ms : this->makeShared) {
        h.append(ms);
    }
    for (StringView es : this->externSelectors) {
        h.append(es);
    }
    return h.get();
}

PLY_NO_INLINE bool BuildFolder::isGenerated(StringView config) const {
    String cmakeListsFolder = BuildFolderName::getFullPath(this->buildFolderName);
    if (!config) {
        config = this->activeConfig;
        if (!config) {
            ErrorHandler::log(
                ErrorHandler::Fatal,
                String::format(
                    "Active config is not set in folder '{}'. Try recreating the folder.\n",
                    this->buildFolderName));
        }
    }

    if (!cmakeBuildSystemExists(cmakeListsFolder, this->cmakeOptions, this->solutionName, config))
        return false;

    u128 previousSig;
    if (isMultiConfigCMakeGenerator(this->cmakeOptions.generator)) {
        previousSig = parseSignatureString(this->buildSystemSignature->text());
    } else {
        previousSig = parseSignatureString(this->buildSystemSignature->get(config)->text());
    }
    return (previousSig == this->currentBuildSystemSignature());
}

PLY_NO_INLINE bool BuildFolder::generate(StringView config,
                                         const ProjectInstantiationResult* instResult) {
    ErrorHandler::log(ErrorHandler::Info,
                      String::format("Generating build system for '{}'...\n", this->solutionName));

    String buildFolderPath = BuildFolderName::getFullPath(this->buildFolderName);
    {
        MemOutStream mout;
        writeCMakeLists(&mout, this->solutionName, buildFolderPath, instResult, false);
        String cmakeListsPath = NativePath::join(buildFolderPath, "CMakeLists.txt");
        FSResult result = FileSystem::native()->makeDirsAndSaveTextIfDifferent(
            cmakeListsPath, mout.moveToString(), TextFormat::platformPreference());
        if (result != FSResult::OK && result != FSResult::Unchanged) {
            ErrorHandler::log(ErrorHandler::Error,
                              String::format("Can't write '{}'\n", cmakeListsPath));
        }
    }

    if (!config) {
        config = this->activeConfig;
        if (!config) {
            ErrorHandler::log(
                ErrorHandler::Fatal,
                String::format(
                    "Active config is not set in folder '{}'. Try recreating the folder.\n",
                    this->buildFolderName));
        }
    }

    Tuple<s32, String> result =
        generateCMakeProject(buildFolderPath, this->cmakeOptions, config, [&](StringView errMsg) {
            ErrorHandler::log(ErrorHandler::Error, errMsg);
        });
    if (result.first != 0) {
        ErrorHandler::log(
            ErrorHandler::Error,
            String::format("Failed to generate build system for '{}':\n", this->solutionName) +
                result.second);
        return false;
    }

    String moduleDefSigStr = signatureToString(this->currentBuildSystemSignature());
    if (isMultiConfigCMakeGenerator(this->cmakeOptions.generator)) {
        this->buildSystemSignature = pylon::Node::createText(moduleDefSigStr.view());
    } else {
        if (!this->buildSystemSignature->isObject()) {
            this->buildSystemSignature = pylon::Node::createObject();
        }
        this->buildSystemSignature->set(config, pylon::Node::createText(moduleDefSigStr.view()));
    }
    this->save();
    return true;
}

struct ConfigListInterpreterHooks : crowbar::Interpreter::Hooks {
    crowbar::Interpreter* interp = nullptr;
    latest::ModuleInstantiator* mi = nullptr;
    BuildFolder* buildFolder = nullptr;
    String currentConfigName;

    virtual ~ConfigListInterpreterHooks() override {
    }

    virtual void enterCustomBlock(const crowbar::Statement::CustomBlock* enteringBlock) override {
        PLY_ASSERT(enteringBlock->type == latest::g_common->configKey);

        // Evaluate config name
        MethodResult result = eval(interp->currentFrame, enteringBlock->expr);
        PLY_ASSERT(result == MethodResult::OK); // FIXME: Make robust
        this->currentConfigName = *interp->returnValue.cast<String>();
        PLY_ASSERT(this->currentConfigName); // FIXME: Make robust

        // Initialize all Module::currentOptions
        for (latest::Repository::Module* mod : latest::Repository::instance->moduleMap) {
            auto newOptions = Owned<latest::Repository::ConfigOptions>::create();
            for (const auto& item : mod->defaultOptions->map) {
                auto cursor = newOptions->map.insertOrFind(item.identifier);
                cursor->obj = AnyOwnedObject::create(item.obj.type);
                cursor->obj.copy(item.obj);
            }
            mod->currentOptions = std::move(newOptions);
        }
    }

    virtual void exitCustomBlock(const crowbar::Statement::CustomBlock* exitingBlock) override {
        PLY_ASSERT(exitingBlock->type == latest::g_common->configKey);

        // Add config to project
        u32 configIndex = this->mi->project.configNames.numItems();
        PLY_ASSERT(configIndex < 64); // FIXME: Handle elegantly
        this->mi->project.configNames.append(this->currentConfigName);

        // Instantiate all root modules in this config
        this->mi->configBit = (u64{1} << configIndex);
        for (StringView targetName : this->buildFolder->rootTargets) {
            buildSteps::Node* rootNode = latest::instantiateModuleForCurrentConfig(
                this->mi, g_labelStorage.insert(targetName));
            PLY_ASSERT(rootNode); // FIXME: Handle elegantly
            if (find(this->mi->project.rootNodes, rootNode) < 0) {
                this->mi->project.rootNodes.append(rootNode);
            }
        }

        // Reset state between configs.
        // Clear currentConfigName, Module::currentOptions, and set ModuleInstantiator status to
        // NotInstantiated.
        this->currentConfigName.clear();
        for (latest::Repository::Module* mod : latest::Repository::instance->moduleMap) {
            mod->currentOptions.clear();
        }
        for (auto& item : this->mi->modules) {
            item.statusInCurrentConfig = latest::ModuleInstantiator::NotInstantiated;
        }
    }
};

struct ConfigNamespace : crowbar::INamespace {
    crowbar::Interpreter* interp = nullptr;

    virtual AnyObject find(Label identifier) const override {
        auto cursor = latest::Repository::instance->moduleMap.find(identifier);
        if (!cursor.wasFound())
            return {};

        // FIXME: Handle gracefully instead of asserting
        // modules should only be looked up within config block
        PLY_ASSERT(this->interp->currentFrame->customBlock->type == latest::g_common->configKey);

        latest::Repository::Module* mod = *cursor;
        return AnyObject::bind(mod->currentOptions.get());
    }
};

PLY_NO_INLINE bool generateLatest(BuildFolder* bf) {
    latest::ModuleInstantiator mi{bf->getAbsPath()};
    mi.project.name = bf->solutionName;

    // Execute the config_list block
    latest::Repository::ConfigList* configList = latest::Repository::instance->configList;
    if (!configList) {
        ErrorHandler::log(ErrorHandler::Fatal, "No config_list block defined.\n");
    }

    {
        // Create new interpreter.
        MemOutStream outs;
        crowbar::Interpreter interp;
        interp.outs = &outs;

        // Add hooks.
        ConfigListInterpreterHooks interpHooks;
        interpHooks.interp = &interp;
        interpHooks.mi = &mi;
        interpHooks.buildFolder = bf;
        interp.hooks = &interpHooks;

        // Add namespace to map module names to ConfigOptions.
        ConfigNamespace configNs;
        configNs.interp = &interp;
        interp.outerNameSpaces.append(&configNs);

        // Add builtin namespace.
        crowbar::MapNamespace builtIns;
        static bool true_ = true;
        static bool false_ = false;
        *builtIns.map.insert(g_labelStorage.insert("true")) = AnyObject::bind(&true_);
        *builtIns.map.insert(g_labelStorage.insert("false")) = AnyObject::bind(&false_);
        interp.outerNameSpaces.append(&builtIns);

        // Invoke block.
        crowbar::Interpreter::StackFrame frame;
        frame.interp = &interp;
        frame.desc = []() -> HybridString { return "config_list"; };
        frame.tkr = &configList->plyfile->tkr;
        MethodResult result = execFunction(&frame, configList->block);
        if (result == MethodResult::Error) {
            StdErr::text() << outs.moveToString();
            exit(1);
        }
    }

    Owned<buildSteps::FlatProject> flatProject = flatten(&mi.project);
    MemOutStream outs;
    writeCMakeLists(&outs, flatProject);

    FileSystem::native()->makeDirsAndSaveTextIfDifferent("out.txt", outs.moveToString(),
                                                         TextFormat::platformPreference());
    return true;
}

PLY_NO_INLINE bool BuildFolder::generateLoop(StringView config) {
    //return generateLatest(this);

    for (;;) {
        ProjectInstantiationResult instResult = this->instantiateAllTargets(false);
        if (!instResult.isValid) {
            return false;
        }
        bool canGenerate = true;
        u32 numUnselected = instResult.unselectedExterns.numItems();
        if (numUnselected > 0) {
            canGenerate = false;
            OutStream outs = StdOut::text();
            for (const DependencySource* unselectedExtern : instResult.unselectedExterns) {
                outs.format("Can't generate build system in folder '{}' because extern '{}' is not "
                            "selected.\n",
                            this->buildFolderName, unselectedExtern->name);
                Array<Tuple<const ExternProvider*, bool>> candidates;
                {
                    for (const ExternProvider* externProvider :
                         RepoRegistry::get()->repo.externProviders) {
                        if (externProvider->extern_ != unselectedExtern)
                            continue;
                        Owned<pylon::Node> toolchain =
                            toolchainInfoFromCMakeOptions(this->cmakeOptions);
                        ExternProviderArgs args;
                        args.toolchain = toolchain;
                        args.provider = externProvider;
                        ExternResult er = externProvider->externFunc(ExternCommand::Status, &args);
                        if (er.isSupported()) {
                            candidates.append({externProvider, er.code == ExternResult::Installed});
                        }
                    }
                }
                if (candidates.isEmpty()) {
                    outs.format("No compatible providers are available for extern '{}'.\n",
                                unselectedExtern->name);
                } else {
                    u32 n = candidates.numItems();
                    outs.format("{} compatible provider{} available:\n", n,
                                n == 1 ? " is" : "s are");
                    for (Tuple<const ExternProvider*, bool> pair : candidates) {
                        outs.format("    {} ({})\n", pair.first->getQualifiedName(),
                                    pair.second ? "installed" : "not installed");
                    }
                }
            }
        }
        if (instResult.uninstalledProviders.numItems() > 0) {
            canGenerate = false;
            OutStream outs = StdOut::text();
            for (const ExternProvider* prov : instResult.uninstalledProviders) {
                outs.format("Can't generate build system in folder '{}' because extern provider "
                            "'{}' is selected, but not installed.\n",
                            this->buildFolderName, prov->getQualifiedName());
            }
        }
        if (canGenerate) {
            // Reinstantiate, but this time pass isGenerating = true:
            instResult = this->instantiateAllTargets(true);
            if (this->generate(config, &instResult)) {
                StdOut::text().format("Successfully generated build system in folder '{}'.\n",
                                      this->buildFolderName);
                return true;
            }
        }
        break;
    }
    return false;
}

PLY_NO_INLINE bool BuildFolder::build(StringView config, StringView targetName,
                                      bool captureOutput) const {
    // Note: Should we check that targetName actually exists in the build folder before invoking
    // CMake? If targetName isn't a root target, this would require us to instaniate all
    // dependencies first.
    if (!config) {
        config = this->activeConfig;
        if (!config) {
            ErrorHandler::log(
                ErrorHandler::Fatal,
                String::format(
                    "Active config is not set in folder '{}'. Try recreating the folder.\n",
                    this->buildFolderName));
        }
    }
    ErrorHandler::log(ErrorHandler::Info,
                      String::format("Building {} configuration of '{}'...\n", config,
                                     targetName ? targetName : this->solutionName.view()));

    String cmakeListsFolder = BuildFolderName::getFullPath(this->buildFolderName);
    Tuple<s32, String> result =
        buildCMakeProject(cmakeListsFolder, this->cmakeOptions, config, targetName, captureOutput);
    if (result.first != 0) {
        ErrorHandler::log(ErrorHandler::Error,
                          String::format("Failed to build '{}':\n",
                                         targetName ? targetName : this->solutionName.view()) +
                              result.second);
        return false;
    }
    return true;
}

PLY_NO_INLINE Array<Owned<BuildFolder>> BuildFolder::getList() {
    Array<Owned<BuildFolder>> result;
    String buildFolderRoot = NativePath::join(PLY_WORKSPACE_FOLDER, "data/build");
    for (const DirectoryEntry& entry : FileSystem::native()->listDir(buildFolderRoot)) {
        if (!entry.isDir)
            continue;
        PLY_ASSERT(!entry.name.isEmpty());
        Owned<BuildFolder> folderInfo = BuildFolder::load(entry.name);
        if (!folderInfo)
            continue;
        result.append(std::move(folderInfo));
    }
    return result;
}

} // namespace build
} // namespace ply

#include "codegen/BuildFolder.inl" //%%
