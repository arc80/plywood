/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-repository/Instantiate.h>
#include <ply-build-provider/ExternFolderRegistry.h>
#include <ply-crowbar/Interpreter.h>
#include <ply-reflect/methods/BoundMethod.h>
#include <ply-runtime/string/WString.h>
#include <ply-runtime/string/TextEncoding.h>
#include <ply-runtime/algorithm/Find.h>
#if PLY_TARGET_WIN32
#include <winhttp.h>
#endif

namespace ply {
namespace build {
namespace latest {

//--------------------------------------------------------------

template <typename T, typename U, typename Callable>
T& appendOrFind(Array<T>& arr, U&& item, const Callable& callable) {
    for (u32 i = 0; i < arr.numItems(); i++) {
        if (callable(arr[i]))
            return arr[i];
    }
    return arr.append(U{std::forward<U>(item)});
}

struct InstantiatingInterpreter {
    crowbar::Interpreter interp;
    ModuleInstantiator* mi = nullptr;
    Repository::ModuleOrFunction* currentModule = nullptr;
    buildSteps::Node* node = nullptr;

    String makeAbsPath(StringView relPath) {
        StringView plyfilePath =
            NativePath::split(this->currentModule->plyfile->tkr.fileLocationMap.path).first;
        return NativePath::join(plyfilePath, relPath);
    }
};

enum class Visibility {
    Error,
    Public,
    Private,
};

Visibility getVisibility(crowbar::Interpreter* interp, const AnyObject& attributes, bool isModule,
                         StringView propertyType) {
    Visibility vis = Visibility::Private;
    s32 tokenIdx = -1;
    if (const StatementAttributes* sa = attributes.cast<StatementAttributes>()) {
        tokenIdx = sa->visibilityTokenIdx;
        if (sa->isPublic) {
            vis = Visibility::Public;
        }
    }
    if (isModule) {
        if (tokenIdx < 0) {
            interp->base.error(
                String::format("{} should have 'public' or 'private' attribute", propertyType));
            return Visibility::Error;
        }
    } else {
        if (tokenIdx >= 0) {
            crowbar::ExpandedToken token = interp->currentFrame->tkr->expandToken(tokenIdx);
            interp->base.error(
                String::format("'{}' cannot be used inside config block", token.text));
            return Visibility::Error;
        }
    }
    return vis;
}

bool assignToCompileOptions(PropertyCollector* pc, const AnyObject& attributes, Label label) {
    Visibility vis = getVisibility(pc->interp, attributes, pc->isModule, "compile options");
    if (vis == Visibility::Error)
        return false;

    buildSteps::ToolchainOpt tcOpt{buildSteps::ToolchainOpt::Type::Generic,
                                   g_labelStorage.view(label),
                                   *pc->interp->base.returnValue.cast<String>()};
    int i = find(pc->node->options, [&](const auto& a) {
        return (a.opt.type == tcOpt.type) && (a.opt.key == tcOpt.key);
    });
    if (i >= 0) {
        pc->node->options.erase(i);
    }
    buildSteps::Node::Option& opt = pc->node->options.append(std::move(tcOpt));
    opt.enabled.bits |= pc->configBit;
    if (vis == Visibility::Public) {
        opt.isPublic.bits |= pc->configBit;
    }
    return true;
}

bool onEvaluateSourceFile(InstantiatingInterpreter* ii, const AnyObject& attributes) {
    PLY_ASSERT(!attributes.data);
    String absPath = ii->makeAbsPath(*ii->interp.base.returnValue.cast<String>());
    buildSteps::Node::SourceGroup& srcGroup = appendOrFind(
        ii->node->sourceGroups, absPath, [&](const auto& a) { return a.absPath == absPath; });
    bool needsCompilation = false;
    for (WalkTriple& triple : FileSystem::native()->walk(absPath, 0)) {
        for (const WalkTriple::FileInfo& file : triple.files) {
            if ((file.name.endsWith(".cpp") && !file.name.endsWith(".modules.cpp")) ||
                file.name.endsWith(".h")) {
                if (!needsCompilation && file.name.endsWith(".cpp")) {
                    needsCompilation = true;
                }
                String relPath =
                    NativePath::makeRelative(absPath, NativePath::join(triple.dirPath, file.name));
                buildSteps::Node::SourceFile& srcFile = appendOrFind(
                    srcGroup.files, std::move(relPath),
                    [&](const buildSteps::Node::SourceFile& a) { return a.relPath == relPath; });
                srcFile.enabled.bits |= ii->mi->configBit;
            }
        }
    }
    if (needsCompilation) {
        ii->node->hasBuildStep.bits |= ii->mi->configBit;
    }
    return true;
}

bool onEvaluateIncludeDirectory(PropertyCollector* pc, const AnyObject& attributes) {
    Visibility vis = getVisibility(pc->interp, attributes, pc->isModule, "include directory");
    if (vis == Visibility::Error)
        return false;

    buildSteps::ToolchainOpt tcOpt{
        buildSteps::ToolchainOpt::Type::IncludeDir,
        NativePath::join(pc->basePath, *pc->interp->base.returnValue.cast<String>())};
    buildSteps::Node::Option& foundOpt = appendOrFind(
        pc->node->options, std::move(tcOpt), [&](const auto& a) { return a.opt == tcOpt; });
    foundOpt.enabled.bits |= pc->configBit;
    if (vis == Visibility::Public) {
        foundOpt.isPublic.bits |= pc->configBit;
    }
    return true;
}

bool onEvaluateDependency(InstantiatingInterpreter* ii, const AnyObject& attributes) {
    const StatementAttributes* sa = attributes.cast<StatementAttributes>();
    PLY_ASSERT(sa->visibilityTokenIdx >= 0);

    // Instantiate the dependency
    Repository::ModuleOrFunction* mod =
        ii->interp.base.returnValue.cast<Repository::ModuleOrFunction>();
    buildSteps::Node* dep =
        instantiateModuleForCurrentConfig(ii->mi, mod->stmt->customBlock()->name);
    if (!dep)
        return true;
    buildSteps::Node::Dependency& foundDep =
        appendOrFind(ii->node->dependencies, dep, [&](const auto& a) { return a.dep == dep; });
    foundDep.enabled.bits |= ii->mi->configBit;
    return true;
}

bool onEvaluateLinkLibrary(PropertyCollector* pc, const AnyObject& attributes) {
    PLY_ASSERT(!attributes.data);
    String* path = pc->interp->base.returnValue.cast<String>();
    buildSteps::Node::LinkerInput& li =
        appendOrFind(pc->node->prebuiltLibs, *path, [&](const auto& a) { return a.path == *path; });
    li.enabled.bits |= pc->configBit;
    return true;
}

MethodResult doCustomBlockInsideConfig(PropertyCollector* pc,
                                       const crowbar::Statement::CustomBlock* cb) {
    crowbar::Interpreter::Hooks hooks;
    if (cb->type == g_common->includeDirectoriesKey) {
        hooks.onEvaluate = {onEvaluateIncludeDirectory, pc};
    } else if (cb->type == g_common->compileOptionsKey) {
        hooks.assignToLocal = {assignToCompileOptions, pc};
    } else if (cb->type == g_common->linkLibrariesKey) {
        hooks.onEvaluate = {onEvaluateLinkLibrary, pc};
    } else {
        // FIXME: Make this a runtime error instead of an assert because the config block can call a
        // function that contains, for example, a dependencies {} block
        PLY_ASSERT(0); // Shouldn't get here
    }
    PLY_SET_IN_SCOPE(pc->interp->currentFrame->hooks, hooks);
    return execBlock(pc->interp->currentFrame, cb->body);
}

MethodResult doCustomBlockAtModuleScope(InstantiatingInterpreter* ii,
                                        const crowbar::Statement::CustomBlock* cb) {
    PropertyCollector pc;
    pc.interp = &ii->interp;
    pc.basePath = NativePath::split(ii->currentModule->plyfile->tkr.fileLocationMap.path).second;
    pc.node = ii->node;
    pc.isModule = true;

    crowbar::Interpreter::Hooks hooks;
    if (cb->type == g_common->sourceFilesKey) {
        hooks.onEvaluate = {onEvaluateSourceFile, ii};
    } else if (cb->type == g_common->includeDirectoriesKey) {
        hooks.onEvaluate = {onEvaluateIncludeDirectory, &pc};
    } else if (cb->type == g_common->compileOptionsKey) {
        hooks.assignToLocal = {assignToCompileOptions, &pc};
    } else if (cb->type == g_common->linkLibrariesKey) {
        hooks.onEvaluate = {onEvaluateLinkLibrary, &pc};
    } else if (cb->type == g_common->dependenciesKey) {
        hooks.onEvaluate = {onEvaluateDependency, ii};
    } else {
        PLY_ASSERT(0); // Shouldn't get here
    }
    PLY_SET_IN_SCOPE(ii->interp.currentFrame->customBlock, cb);
    PLY_SET_IN_SCOPE(ii->interp.currentFrame->hooks, hooks);
    return execBlock(ii->interp.currentFrame, cb->body);
}

MethodResult doJoinPath(const MethodArgs& args) {
    Array<StringView> parts;
    parts.reserve(args.args.numItems);
    for (const AnyObject& arg : args.args) {
        if (!arg.is<String>())
            PLY_FORCE_CRASH();
        parts.append(*arg.cast<String>());
    }
    String result = PathFormat{false}.joinAndNormalize(parts);
    AnyObject* resultStorage =
        args.base->localVariableStorage.appendObject(getTypeDescriptor(&result));
    *resultStorage->cast<String>() = std::move(result);
    args.base->returnValue = *resultStorage;
    return MethodResult::OK;
}

MethodResult getExternFolder(const MethodArgs& args) {
    if (args.args.numItems != 1) {
        args.base->error(String::format("'get_extern_folder' expects 1 argument"));
        return MethodResult::Error;
    }
    String* name = args.args[0].safeCast<String>();
    if (!name) {
        args.base->error(String::format("'get_extern_folder' argument must be a string"));
        return MethodResult::Error;
    }

    ExternFolder* externFolder = ExternFolderRegistry::get()->find(*name);
    if (!externFolder) {
        externFolder = ExternFolderRegistry::get()->create(*name);
        externFolder->save();
    }

    AnyObject* resultStorage =
        args.base->localVariableStorage.appendObject(getTypeDescriptor<String>());
    *resultStorage->cast<String>() = externFolder->path;
    args.base->returnValue = *resultStorage;
    return MethodResult::OK;
}

struct ReadOnlyDict {
    String name;
    LabelMap<AnyObject> map;

    ReadOnlyDict(StringView name) : name{name} {
    }
};

} // namespace latest
} // namespace build
PLY_DECLARE_TYPE_DESCRIPTOR(build::latest::ReadOnlyDict)
namespace build {
namespace latest {

MethodResult sys_fs_exists(const MethodArgs& args) {
    if (args.args.numItems != 1) {
        args.base->error(String::format("'exists' expects 1 argument; got {}", args.args.numItems));
        return MethodResult::Error;
    }
    String* path = args.args[0].safeCast<String>();
    if (!path) {
        args.base->error(String::format("'exists' argument must be a string"));
        return MethodResult::Error;
    }

    ExistsResult result = FileSystem::native()->exists(*path);
    AnyObject* resultStorage =
        args.base->localVariableStorage.appendObject(getTypeDescriptor<bool>());
    *resultStorage->cast<bool>() = (result != ExistsResult::NotFound);
    args.base->returnValue = *resultStorage;
    return MethodResult::OK;
}

struct SplitURL {
    bool https = false;
    String hostname;
    String resource;
};

bool splitURL(BaseInterpreter* base, SplitURL* split, StringView s) {
    if (s.startsWith("https://")) {
        split->https = true;
        s.offsetHead(8);
    } else if (s.startsWith("http://")) {
        s.offsetHead(7);
    } else {
        base->error("URL must start with 'http://' or 'https://'");
        return false;
    }

    s32 i = s.findByte('/');
    if (i < 0) {
        base->error("Expected '/' after hostname");
        return false;
    }

    split->hostname = s.left(i);
    split->resource = s.subStr(i);
    return true;
}

PLY_NO_INLINE WString toWString(StringView str) {
    MemOutStream outs;
    while (str.numBytes > 0) {
        DecodeResult decoded = UTF8::decodePoint(str);
        outs.makeBytesAvailable(4);
        u32 numEncodedBytes = UTF16_Native::encodePoint(outs.viewAvailable(), decoded.point);
        outs.curByte += numEncodedBytes;
        str.offsetHead(decoded.numBytes);
    }
    NativeEndianWriter{&outs}.write<u16>(0);
    return WString::moveFromString(outs.moveToString());
}

void download(StringView dstPath, const SplitURL& split) {
#if PLY_TARGET_WIN32
    // FIXME: More graceful error handling
    // This could also be an InPipe
    HINTERNET hsess = WinHttpOpen(L"PlywoodSDK/0.1", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                  WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    PLY_ASSERT(hsess);
    HINTERNET hconn =
        WinHttpConnect(hsess, toWString(split.hostname),
                       split.https ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT, 0);
    PLY_ASSERT(hconn);
    HINTERNET hreq =
        WinHttpOpenRequest(hconn, L"GET", toWString(split.resource), NULL, WINHTTP_NO_REFERER,
                           WINHTTP_DEFAULT_ACCEPT_TYPES, split.https ? WINHTTP_FLAG_SECURE : 0);
    PLY_ASSERT(hreq);
    BOOL rc = WinHttpSendRequest(hreq, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0,
                                 0, 0);
    PLY_ASSERT(rc);
    rc = WinHttpReceiveResponse(hreq, NULL);
    PLY_ASSERT(rc);

    Owned<OutStream> outs = FileSystem::native()->openStreamForWrite(dstPath);
    PLY_ASSERT(outs);
    for (;;) {
        DWORD size = 0;
        rc = WinHttpQueryDataAvailable(hreq, &size);
        PLY_ASSERT(rc);
        if (size == 0)
            break;

        outs->tryMakeBytesAvailable();
        MutableStringView dst = outs->viewAvailable();
        DWORD downloaded = 0;
        rc = WinHttpReadData(hreq, (LPVOID) dst.bytes, dst.numBytes, &downloaded);
        PLY_ASSERT(rc);
        PLY_ASSERT(downloaded <= dst.numBytes);
        outs->curByte += downloaded;
    }

    WinHttpCloseHandle(hreq);
    WinHttpCloseHandle(hconn);
    WinHttpCloseHandle(hsess);
#else
    PLY_FORCE_CRASH(0); // Not implemented yet
#endif
}

MethodResult sys_fs_download(const MethodArgs& args) {
    if (args.args.numItems != 2) {
        args.base->error(
            String::format("'download' expects 2 arguments; got {}", args.args.numItems));
        return MethodResult::Error;
    }
    String* path = args.args[0].safeCast<String>();
    if (!path) {
        args.base->error(String::format("first argument to 'download' must be a string"));
        return MethodResult::Error;
    }
    String* url = args.args[1].safeCast<String>();
    if (!url) {
        args.base->error(String::format("second argument to 'download' must be a string"));
        return MethodResult::Error;
    }

    StringView s = *url;
    SplitURL split;
    if (!splitURL(args.base, &split, *url))
        return MethodResult::Error;
    download(*path, split);
    args.base->returnValue = {};
    return MethodResult::OK;
}

PLY_NO_INLINE MethodTable getMethodTable_ReadOnlyDict() {
    MethodTable methods;
    methods.propertyLookup = [](BaseInterpreter* interp, const AnyObject& obj,
                                StringView propertyName) -> MethodResult {
        auto* dict = obj.cast<ReadOnlyDict>();
        Label label = g_labelStorage.find(propertyName);
        if (label) {
            AnyObject* prop = dict->map.find(label);
            if (prop) {
                if (prop->is<Method>()) {
                    AnyObject* bm =
                        interp->localVariableStorage.appendObject(getTypeDescriptor<BoundMethod>());
                    *bm->cast<BoundMethod>() = {obj, *prop};
                    interp->returnValue = *bm;
                } else {
                    interp->returnValue = *prop;
                }
                return MethodResult::OK;
            }
        }

        interp->returnValue = {};
        interp->error(String::format("property '{}' not found in '{}'", propertyName, dict->name));
        return MethodResult::Error;
    };
    return methods;
}

void inherit(Array<buildSteps::Node::Option>& dstOpts, const buildSteps::Node::Option& srcOpt) {
    s32 i = find(dstOpts, [&](const buildSteps::Node::Option& o) { return o.opt == srcOpt.opt; });
    if (i < 0) {
        i = dstOpts.numItems();
        dstOpts.append({srcOpt.opt, 0, 0});
    }
    dstOpts[i].enabled.bits |= srcOpt.enabled.bits;
}

buildSteps::Node* instantiateModuleForCurrentConfig(ModuleInstantiator* mi, Label moduleLabel) {
    StringView moduleName = g_labelStorage.view(moduleLabel);

    // Check if a buildSteps::Node was already created for this moduleName.
    buildSteps::Node* node;
    {
        auto instCursor = mi->modules.insertOrFind(moduleName);
        if (!instCursor.wasFound()) {
            // No. Create a new one.
            instCursor->node = new buildSteps::Node;
            instCursor->node->name = moduleName;
        } else {
            // Yes. If the module was already fully instantiated in this config, return it.
            if (instCursor->statusInCurrentConfig == ModuleInstantiator::Instantiated)
                return instCursor->node;
            // Circular dependency check. FIXME: Handle gracefully
            if (instCursor->statusInCurrentConfig == ModuleInstantiator::Instantiating) {
                PLY_FORCE_CRASH();
            }
            PLY_ASSERT(instCursor->statusInCurrentConfig == ModuleInstantiator::NotInstantiated);
        }
        // Set this module's status as Instantiating so that circular dependencies can be detected.
        instCursor->statusInCurrentConfig = ModuleInstantiator::Instantiating;
        node = instCursor->node;
    }

    // Set node as active in this config.
    PLY_ASSERT(mi->configBit);
    node->enabled.bits |= mi->configBit;

    // Initialize node properties
    for (const buildSteps::Node::Option& srcOpt : mi->initNode->options) {
        inherit(node->options, srcOpt);
    }

    // Find module function by name.
    latest::Repository::ModuleOrFunction** mod = g_repository->globalScope.find(moduleLabel);
    if (!mod || !(*mod)->stmt->customBlock()) {
        PLY_FORCE_CRASH(); // FIXME: Handle gracefully
    }
    const crowbar::Statement::CustomBlock* moduleDef = (*mod)->stmt->customBlock().get();
    if (moduleDef->type == g_common->executableKey) {
        node->type = buildSteps::Node::Type::Executable;
    } else if (moduleDef->type == g_common->moduleKey) {
        node->type = buildSteps::Node::Type::Lib;
    } else {
        PLY_ASSERT(0);
    }

    // Create new interpreter.
    InstantiatingInterpreter ii;
    ii.interp.base.error = [&ii](StringView message) {
        OutStream outs = StdErr::text();
        logErrorWithStack(&outs, &ii.interp, message);
    };
    ii.mi = mi;
    ii.currentModule = *mod;
    ii.node = node;

    // Populate global & module namespaces.
    LabelMap<AnyObject> builtIns;
    bool true_ = true;
    bool false_ = false;
    *builtIns.insert(g_labelStorage.insert("true")) = AnyObject::bind(&true_);
    *builtIns.insert(g_labelStorage.insert("false")) = AnyObject::bind(&false_);
    *builtIns.insert(g_labelStorage.insert("join_path")) = AnyObject::bind(doJoinPath);
    *builtIns.insert(g_labelStorage.insert("build_folder")) =
        AnyObject::bind(&ii.mi->buildFolderPath);
    ReadOnlyDict dict_build{"build"};
    String value_arch = "x64";
    *dict_build.map.insert(g_labelStorage.insert("arch")) = AnyObject::bind(&value_arch);
    *builtIns.insert(g_labelStorage.insert("build")) = AnyObject::bind(&dict_build);
    ReadOnlyDict dict_sys{"sys"};
    *dict_sys.map.insert(g_labelStorage.insert("get_extern_folder")) =
        AnyObject::bind(getExternFolder);
    *dict_sys.map.insert(g_labelStorage.insert("download")) = AnyObject::bind(&sys_fs_download);
    *builtIns.insert(g_labelStorage.insert("sys")) = AnyObject::bind(&dict_sys);
    ReadOnlyDict dict_sys_fs{"sys.fs"};
    *dict_sys_fs.map.insert(g_labelStorage.insert("exists")) = AnyObject::bind(&sys_fs_exists);
    *dict_sys.map.insert(g_labelStorage.insert("fs")) = AnyObject::bind(&dict_sys_fs);
    AnyObject::bind(&ii.mi->buildFolderPath);
    ii.interp.resolveName = [&builtIns, &ii](Label identifier) -> AnyObject {
        if (AnyObject* builtIn = builtIns.find(identifier))
            return *builtIn;
        if (AnyObject* obj = ii.currentModule->currentOptions->map.find(identifier))
            return *obj;
        if (latest::Repository::ModuleOrFunction** mod =
                latest::g_repository->globalScope.find(identifier)) {
            if (auto fnDef = (*mod)->stmt->functionDefinition())
                return AnyObject::bind(fnDef.get());
            else
                return AnyObject::bind(*mod);
        }
        return {};
    };

    // Invoke module function.
    crowbar::Interpreter::StackFrame frame;
    frame.hooks.doCustomBlock = {doCustomBlockAtModuleScope, &ii};
    frame.interp = &ii.interp;
    frame.desc = [moduleDef]() -> HybridString {
        return String::format("module '{}'", g_labelStorage.view(moduleDef->name));
    };
    frame.tkr = &(*mod)->plyfile->tkr;
    MethodResult result = execFunction(&frame, moduleDef->body);
    if (result == MethodResult::Error)
        return nullptr;

    mi->modules.find(moduleName)->statusInCurrentConfig = ModuleInstantiator::Instantiated;
    return node;
}

TypeKey TypeKey_ReadOnlyDict{
    // getName
    [](const TypeDescriptor* typeDesc) -> HybridString { //
        return "ReadOnlyDict";
    },

    // write
    nullptr, // Unimplemented

    // writeFormat
    nullptr, // Unimplemented

    // read
    nullptr, // Unimplemented

    // hashDescriptor
    TypeKey::hashEmptyDescriptor,

    // equalDescriptors
    TypeKey::alwaysEqualDescriptors,
};

} // namespace latest
} // namespace build
} // namespace ply

PLY_DEFINE_TYPE_DESCRIPTOR(ply::build::latest::ReadOnlyDict) {
    static TypeDescriptor typeDesc{
        &ply::build::latest::TypeKey_ReadOnlyDict, (ply::build::latest::ReadOnlyDict*) nullptr,
        NativeBindings::make<build::latest::ReadOnlyDict>()
            PLY_METHOD_TABLES_ONLY(, build::latest::getMethodTable_ReadOnlyDict())};
    return &typeDesc;
}
