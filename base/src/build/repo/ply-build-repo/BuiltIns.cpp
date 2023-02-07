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

FnResult do_join_path(const FnParams& params) {
    Array<StringView> parts;
    parts.reserve(params.args.num_items);
    for (const AnyObject& arg : params.args) {
        if (!arg.is<String>())
            PLY_FORCE_CRASH();
        parts.append(*arg.cast<String>());
    }
    String result = Path.join_array(parts);
    AnyObject* result_storage =
        params.base->local_variable_storage.append_object(get_type_descriptor(&result));
    *result_storage->cast<String>() = std::move(result);
    params.base->return_value = *result_storage;
    return Fn_OK;
}

FnResult do_escape(const FnParams& params) {
    if (params.args.num_items != 1) {
        params.base->error(String::format("'escape' expects 1 argument"));
        return Fn_Error;
    }
    String* path = params.args[0].safe_cast<String>();
    if (!path) {
        params.base->error(String::format("'escape' argument 1 must be a string"));
        return Fn_Error;
    }

    String result = to_string(escape(*path));
    AnyObject* result_storage = params.base->local_variable_storage.append_object(
        get_type_descriptor<String>());
    *result_storage->cast<String>() = std::move(result);
    params.base->return_value = *result_storage;
    return Fn_OK;
}

FnResult do_save_if_different(const FnParams& params) {
    if (params.args.num_items != 2) {
        params.base->error(String::format("'save_if_different' expects 2 arguments"));
        return Fn_Error;
    }
    String* path = params.args[0].safe_cast<String>();
    if (!path) {
        params.base->error(
            String::format("'save_if_different' argument 1 must be a string"));
        return Fn_Error;
    }
    String* content = params.args[1].safe_cast<String>();
    if (!content) {
        params.base->error(
            String::format("'save_if_different' argument 2 must be a string"));
        return Fn_Error;
    }

    FileSystem.make_dirs_and_save_binary_if_different(*path, *content);
    return Fn_OK;
}

FnResult get_extern_folder(const FnParams& params) {
    if (params.args.num_items != 1) {
        params.base->error(String::format("'get_extern_folder' expects 1 argument"));
        return Fn_Error;
    }
    String* name = params.args[0].safe_cast<String>();
    if (!name) {
        params.base->error(
            String::format("'get_extern_folder' argument must be a string"));
        return Fn_Error;
    }

    ExternFolder* extern_folder = ExternFolderRegistry::get()->find(*name);
    if (!extern_folder) {
        extern_folder = ExternFolderRegistry::get()->create(*name);
        extern_folder->save();
    }

    AnyObject* result_storage = params.base->local_variable_storage.append_object(
        get_type_descriptor<String>());
    *result_storage->cast<String>() = extern_folder->path;
    params.base->return_value = *result_storage;
    return Fn_OK;
}

FnResult sys_fs_exists(const FnParams& params) {
    if (params.args.num_items != 1) {
        params.base->error(String::format("'exists' expects 1 argument; got {}",
                                          params.args.num_items));
        return Fn_Error;
    }
    String* path = params.args[0].safe_cast<String>();
    if (!path) {
        params.base->error(String::format("'exists' argument must be a string"));
        return Fn_Error;
    }

    ExistsResult result = FileSystem.exists(*path);
    AnyObject* result_storage =
        params.base->local_variable_storage.append_object(get_type_descriptor<bool>());
    *result_storage->cast<bool>() = (result != ExistsResult::NotFound);
    params.base->return_value = *result_storage;
    return Fn_OK;
}

struct SplitURL {
    bool https = false;
    String hostname;
    String resource;
};

bool split_url(BaseInterpreter* base, SplitURL* split, StringView s) {
    if (s.starts_with("https://")) {
        split->https = true;
        s.offset_head(8);
    } else if (s.starts_with("http://")) {
        s.offset_head(7);
    } else {
        base->error("URL must start with 'http://' or 'https://'");
        return false;
    }

    s32 i = s.find_byte('/');
    if (i < 0) {
        base->error("Expected '/' after hostname");
        return false;
    }

    split->hostname = s.left(i);
    split->resource = s.sub_str(i);
    return true;
}

void download(StringView dst_path, const SplitURL& split) {
#if PLY_TARGET_WIN32
    // FIXME: More graceful error handling
    // This could also be an InPipe
    HINTERNET hsess = WinHttpOpen(L"PlywoodSDK/0.1", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                  WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    PLY_ASSERT(hsess);
    HINTERNET hconn = WinHttpConnect(
        hsess, to_wstring(split.hostname),
        split.https ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT, 0);
    PLY_ASSERT(hconn);
    HINTERNET hreq = WinHttpOpenRequest(
        hconn, L"GET", to_wstring(split.resource), NULL, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES, split.https ? WINHTTP_FLAG_SECURE : 0);
    PLY_ASSERT(hreq);
    BOOL rc = WinHttpSendRequest(hreq, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                 WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    PLY_ASSERT(rc);
    rc = WinHttpReceiveResponse(hreq, NULL);
    PLY_ASSERT(rc);

    OutStream out = FileSystem.open_stream_for_write(dst_path);
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
        rc = WinHttpReadData(hreq, (LPVOID) dst.bytes, dst.num_bytes, &downloaded);
        PLY_ASSERT(rc);
        PLY_ASSERT(downloaded <= dst.num_bytes);
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
    if (params.args.num_items != 2) {
        params.base->error(String::format("'download' expects 2 arguments; got {}",
                                          params.args.num_items));
        return Fn_Error;
    }
    String* path = params.args[0].safe_cast<String>();
    if (!path) {
        params.base->error(
            String::format("first argument to 'download' must be a string"));
        return Fn_Error;
    }
    String* url = params.args[1].safe_cast<String>();
    if (!url) {
        params.base->error(
            String::format("second argument to 'download' must be a string"));
        return Fn_Error;
    }

    StringView s = *url;
    SplitURL split;
    if (!split_url(params.base, &split, *url))
        return Fn_Error;
    download(*path, split);
    params.base->return_value = {};
    return Fn_OK;
}

MethodTable get_method_table_read_only_dict() {
    MethodTable methods;
    methods.property_lookup = [](BaseInterpreter* interp, const AnyObject& obj,
                                 StringView property_name) -> FnResult {
        auto* dict = obj.cast<ReadOnlyDict>();
        Label label = g_labelStorage.find(property_name);
        if (label) {
            AnyObject* prop = dict->map.find(label);
            if (prop) {
                if (prop->is<NativeFunction>()) {
                    AnyObject* bm = interp->local_variable_storage.append_object(
                        get_type_descriptor<BoundMethod>());
                    *bm->cast<BoundMethod>() = {obj, *prop};
                    interp->return_value = *bm;
                } else {
                    interp->return_value = *prop;
                }
                return Fn_OK;
            }
        }

        interp->return_value = {};
        interp->error(String::format("property '{}' not found in '{}'", property_name,
                                     dict->name));
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
    BuiltInMap.assign(g_labelStorage.insert("false"),
                      AnyObject::bind(&BuiltInStorage.false_));
    BuiltInMap.assign(g_labelStorage.insert("join_path"),
                      AnyObject::bind(do_join_path));
    BuiltInMap.assign(g_labelStorage.insert("script_path"),
                      AnyObject::bind(&BuiltInStorage.script_path));
    BuiltInMap.assign(g_labelStorage.insert("escape"), AnyObject::bind(do_escape));
    BuiltInMap.assign(g_labelStorage.insert("save_if_different"),
                      AnyObject::bind(do_save_if_different));

    // sys dictionary
    BuiltInStorage.dict_sys.map.assign(
        g_labelStorage.insert("target_platform"),
        AnyObject::bind(&BuiltInStorage.sys_target_platform));
    BuiltInStorage.dict_sys.map.assign(
        g_labelStorage.insert("target_arch"),
        AnyObject::bind(&BuiltInStorage.sys_target_arch));
    PLY_ASSERT(build_folder->abs_path);
    BuiltInStorage.dict_sys.map.assign(g_labelStorage.insert("build_folder"),
                                       AnyObject::bind(&build_folder->abs_path));
    BuiltInStorage.dict_sys.map.assign(g_labelStorage.insert("cmake_path"),
                                       AnyObject::bind(&BuiltInStorage.sys_cmake_path));
    BuiltInStorage.dict_sys.map.assign(g_labelStorage.insert("get_extern_folder"),
                                       AnyObject::bind(get_extern_folder));
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
    // get_name
    [](const TypeDescriptor* type_desc) -> HybridString { //
        return "ReadOnlyDict";
    },

    // hash_descriptor
    TypeKey::hash_empty_descriptor,

    // equal_descriptors
    TypeKey::always_equal_descriptors,
};

} // namespace build
} // namespace ply

PLY_DEFINE_TYPE_DESCRIPTOR(ply::build::ReadOnlyDict) {
    static TypeDescriptor type_desc{
        &ply::build::TypeKey_ReadOnlyDict, (ply::build::ReadOnlyDict*) nullptr,
        NativeBindings::make<ply::build::ReadOnlyDict>()
            PLY_METHOD_TABLES_ONLY(, ply::build::get_method_table_read_only_dict())};
    return &type_desc;
}
