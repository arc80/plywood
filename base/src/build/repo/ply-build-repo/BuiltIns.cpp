/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-build-repo/BuiltIns.h>
#include <ply-reflect/methods/BoundMethod.h>
#include <ply-runtime/string/WString.h>
#include <ply-build-repo/ExternFolderRegistry.h>
#include <ply-build-repo/BuildFolder.h>
#if PLY_TARGET_WIN32
#include <winhttp.h>
#endif

namespace ply {
namespace build {

BuiltInStorage_ BuiltInStorage;
Map<Label, AnyObject> BuiltInMap;

FnResult doJoinPath(const FnParams& params) {
    Array<StringView> parts;
    parts.reserve(params.args.numItems);
    for (const AnyObject& arg : params.args) {
        if (!arg.is<String>())
            PLY_FORCE_CRASH();
        parts.append(*arg.cast<String>());
    }
    String result = Path.joinArray(parts);
    AnyObject* resultStorage =
        params.base->localVariableStorage.appendObject(getTypeDescriptor(&result));
    *resultStorage->cast<String>() = std::move(result);
    params.base->returnValue = *resultStorage;
    return Fn_OK;
}

FnResult doEscape(const FnParams& params) {
    if (params.args.numItems != 1) {
        params.base->error(String::format("'escape' expects 1 argument"));
        return Fn_Error;
    }
    String* path = params.args[0].safeCast<String>();
    if (!path) {
        params.base->error(String::format("'escape' argument 1 must be a string"));
        return Fn_Error;
    }

    String result = to_string(escape(*path));
    AnyObject* resultStorage =
        params.base->localVariableStorage.appendObject(getTypeDescriptor<String>());
    *resultStorage->cast<String>() = std::move(result);
    params.base->returnValue = *resultStorage;
    return Fn_OK;
}

FnResult doSaveIfDifferent(const FnParams& params) {
    if (params.args.numItems != 2) {
        params.base->error(String::format("'save_if_different' expects 2 arguments"));
        return Fn_Error;
    }
    String* path = params.args[0].safeCast<String>();
    if (!path) {
        params.base->error(String::format("'save_if_different' argument 1 must be a string"));
        return Fn_Error;
    }
    String* content = params.args[1].safeCast<String>();
    if (!content) {
        params.base->error(String::format("'save_if_different' argument 2 must be a string"));
        return Fn_Error;
    }

    FileSystem.makeDirsAndSaveBinaryIfDifferent(*path, *content);
    return Fn_OK;
}

FnResult getExternFolder(const FnParams& params) {
    if (params.args.numItems != 1) {
        params.base->error(String::format("'get_extern_folder' expects 1 argument"));
        return Fn_Error;
    }
    String* name = params.args[0].safeCast<String>();
    if (!name) {
        params.base->error(String::format("'get_extern_folder' argument must be a string"));
        return Fn_Error;
    }

    ExternFolder* externFolder = ExternFolderRegistry::get()->find(*name);
    if (!externFolder) {
        externFolder = ExternFolderRegistry::get()->create(*name);
        externFolder->save();
    }

    AnyObject* resultStorage =
        params.base->localVariableStorage.appendObject(getTypeDescriptor<String>());
    *resultStorage->cast<String>() = externFolder->path;
    params.base->returnValue = *resultStorage;
    return Fn_OK;
}

FnResult sys_fs_exists(const FnParams& params) {
    if (params.args.numItems != 1) {
        params.base->error(String::format("'exists' expects 1 argument; got {}", params.args.numItems));
        return Fn_Error;
    }
    String* path = params.args[0].safeCast<String>();
    if (!path) {
        params.base->error(String::format("'exists' argument must be a string"));
        return Fn_Error;
    }

    ExistsResult result = FileSystem.exists(*path);
    AnyObject* resultStorage =
        params.base->localVariableStorage.appendObject(getTypeDescriptor<bool>());
    *resultStorage->cast<bool>() = (result != ExistsResult::NotFound);
    params.base->returnValue = *resultStorage;
    return Fn_OK;
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

    OutStream out = FileSystem.openStreamForWrite(dstPath);
    PLY_ASSERT(out);
    for (;;) {
        DWORD size = 0;
        rc = WinHttpQueryDataAvailable(hreq, &size);
        PLY_ASSERT(rc);
        if (size == 0)
            break;

        out.ensure_writable();
        MutStringView dst = out.view_writable();
        DWORD downloaded = 0;
        rc = WinHttpReadData(hreq, (LPVOID) dst.bytes, dst.numBytes, &downloaded);
        PLY_ASSERT(rc);
        PLY_ASSERT(downloaded <= dst.numBytes);
        out.cur_byte += downloaded;
    }

    WinHttpCloseHandle(hreq);
    WinHttpCloseHandle(hconn);
    WinHttpCloseHandle(hsess);
#else
    PLY_FORCE_CRASH(0); // Not implemented yet
#endif
}

FnResult sys_fs_download(const FnParams& params) {
    if (params.args.numItems != 2) {
        params.base->error(
            String::format("'download' expects 2 arguments; got {}", params.args.numItems));
        return Fn_Error;
    }
    String* path = params.args[0].safeCast<String>();
    if (!path) {
        params.base->error(String::format("first argument to 'download' must be a string"));
        return Fn_Error;
    }
    String* url = params.args[1].safeCast<String>();
    if (!url) {
        params.base->error(String::format("second argument to 'download' must be a string"));
        return Fn_Error;
    }

    StringView s = *url;
    SplitURL split;
    if (!splitURL(params.base, &split, *url))
        return Fn_Error;
    download(*path, split);
    params.base->returnValue = {};
    return Fn_OK;
}

PLY_NO_INLINE MethodTable getMethodTable_ReadOnlyDict() {
    MethodTable methods;
    methods.propertyLookup = [](BaseInterpreter* interp, const AnyObject& obj,
                                StringView propertyName) -> FnResult {
        auto* dict = obj.cast<ReadOnlyDict>();
        Label label = g_labelStorage.find(propertyName);
        if (label) {
            AnyObject* prop = dict->map.find(label);
            if (prop) {
                if (prop->is<NativeFunction>()) {
                    AnyObject* bm =
                        interp->localVariableStorage.appendObject(getTypeDescriptor<BoundMethod>());
                    *bm->cast<BoundMethod>() = {obj, *prop};
                    interp->returnValue = *bm;
                } else {
                    interp->returnValue = *prop;
                }
                return Fn_OK;
            }
        }

        interp->returnValue = {};
        interp->error(String::format("property '{}' not found in '{}'", propertyName, dict->name));
        return Fn_Error;
    };
    return methods;
}

void init_built_ins(BuildFolder_t* build_folder) {
    PLY_ASSERT(BuiltInMap.num_items() == 0);
    bool true_ = true;
    bool false_ = false;
    BuiltInStorage.sys_target_platform = "windows";
    BuiltInStorage.sys_target_arch = "x64";
    BuiltInStorage.dict_build.name = "build";
    BuiltInStorage.dict_sys.name = "sys";
    BuiltInStorage.dict_sys_fs.name = "sys.fs";

    BuiltInMap.assign(g_labelStorage.insert("true"),
                      AnyObject::bind(&BuiltInStorage.true_));
    BuiltInMap.assign(g_labelStorage.insert("false"), AnyObject::bind(&BuiltInStorage.false_));
    BuiltInMap.assign(g_labelStorage.insert("join_path"), AnyObject::bind(doJoinPath));
    BuiltInMap.assign(g_labelStorage.insert("script_path"),
                      AnyObject::bind(&BuiltInStorage.script_path));
    BuiltInMap.assign(g_labelStorage.insert("escape"), AnyObject::bind(doEscape));
    BuiltInMap.assign(g_labelStorage.insert("save_if_different"),
                      AnyObject::bind(doSaveIfDifferent));

    // sys dictionary
    BuiltInStorage.dict_sys.map.assign(
        g_labelStorage.insert("target_platform"),
        AnyObject::bind(&BuiltInStorage.sys_target_platform));
    BuiltInStorage.dict_sys.map.assign(
        g_labelStorage.insert("target_arch"),
        AnyObject::bind(&BuiltInStorage.sys_target_arch));
    PLY_ASSERT(build_folder->absPath);
    BuiltInStorage.dict_sys.map.assign(g_labelStorage.insert("build_folder"),
                                       AnyObject::bind(&build_folder->absPath));
    BuiltInStorage.dict_sys.map.assign(g_labelStorage.insert("cmake_path"),
                                       AnyObject::bind(&BuiltInStorage.sys_cmake_path));
    BuiltInStorage.dict_sys.map.assign(g_labelStorage.insert("get_extern_folder"),
                                       AnyObject::bind(getExternFolder));
    BuiltInStorage.dict_sys.map.assign(g_labelStorage.insert("download"),
                                       AnyObject::bind(&sys_fs_download));
    BuiltInMap.assign(g_labelStorage.insert("sys"),
                      AnyObject::bind(&BuiltInStorage.dict_sys));

    // sys.fs
    BuiltInStorage.dict_sys_fs.map.assign(g_labelStorage.insert("exists"),
                                          AnyObject::bind(&sys_fs_exists));
    BuiltInStorage.dict_sys.map.assign(g_labelStorage.insert("fs"),
                                       AnyObject::bind(&BuiltInStorage.dict_sys_fs));
}

TypeKey TypeKey_ReadOnlyDict{
    // getName
    [](const TypeDescriptor* typeDesc) -> HybridString { //
        return "ReadOnlyDict";
    },

    // hashDescriptor
    TypeKey::hashEmptyDescriptor,

    // equalDescriptors
    TypeKey::alwaysEqualDescriptors,
};

} // namespace build
} // namespace ply

PLY_DEFINE_TYPE_DESCRIPTOR(ply::build::ReadOnlyDict) {
    static TypeDescriptor typeDesc{
        &ply::build::TypeKey_ReadOnlyDict, (ply::build::ReadOnlyDict*) nullptr,
        NativeBindings::make<ply::build::ReadOnlyDict>()
            PLY_METHOD_TABLES_ONLY(, ply::build::getMethodTable_ReadOnlyDict())};
    return &typeDesc;
}
