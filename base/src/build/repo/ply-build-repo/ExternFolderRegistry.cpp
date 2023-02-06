/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-build-repo/Core.h>
#include <ply-build-repo/ExternFolderRegistry.h>
#include <ply-build-repo/Workspace.h>
#include <pylon/Parse.h>
#include <pylon/Write.h>
#include <pylon-reflect/Import.h>
#include <pylon-reflect/Export.h>

namespace ply {
namespace build {

Owned<ExternFolderRegistry> ExternFolderRegistry::instance_;

PLY_NO_INLINE Owned<ExternFolder> ExternFolder::load(String&& path) {
    String info_path = Path.join(path, "info.pylon");
    String str_contents = FileSystem.load_text_autodetect(info_path);
    if (FileSystem.last_result() != FSResult::OK)
        return nullptr;

    auto a_root = pylon::Parser{}.parse(info_path, str_contents).root;
    if (!a_root->is_valid())
        return nullptr;

    Owned<ExternFolder> info = pylon::import <ExternFolder>(a_root);
    info->path = std::move(path);

    return info;
}

PLY_NO_INLINE bool ExternFolder::save() const {
    auto a_root = pylon::export_obj(AnyObject::bind(this));
    String str_contents = pylon::to_string(a_root);
    String info_path = Path.join(this->path, "info.pylon");
    FSResult rc =
        FileSystem.make_dirs_and_save_text_if_different(info_path, str_contents);
    return (rc == FSResult::OK || rc == FSResult::Unchanged);
}

Owned<ExternFolderRegistry> ExternFolderRegistry::create() {
    PLY_ASSERT(!instance_);
    Owned<ExternFolderRegistry> extern_folders = new ExternFolderRegistry;
    extern_folders->folders.clear();
    String build_folder_root = Path.join(Workspace.path, "data/extern");
    // FIXME: Use native() or compat() consistently
    // FIXME: Detect and report duplicate extern folders
    for (const FileInfo& entry : FileSystem.list_dir(build_folder_root, 0)) {
        if (!entry.is_dir)
            continue;
        PLY_ASSERT(!entry.name.is_empty());
        String folder_path = Path.join(build_folder_root, entry.name);
        Owned<ExternFolder> folder_info = ExternFolder::load(std::move(folder_path));
        if (!folder_info)
            continue;
        extern_folders->folders.append(std::move(folder_info));
    }
    return extern_folders;
}

ExternFolder* ExternFolderRegistry::find(StringView desc) const {
    for (ExternFolder* info : this->folders) {
        if (info->desc == desc)
            return info;
    }
    return nullptr;
}

PLY_NO_INLINE String make_unique_file_name(StringView parent_folder,
                                           StringView prefix) {
    u32 number = 0;
    String suffix;
    for (;;) {
        String path = Path.join(parent_folder, prefix + suffix);
        if (FileSystem.exists(path) == ExistsResult::NotFound)
            return path;
        number++;
        suffix = to_string(number);
        u32 num_zero_digits = max<s32>(3 - suffix.num_bytes, 0);
        suffix = String::format(".{}{}", StringView{"0"} * num_zero_digits, suffix);
    }
}

PLY_NO_INLINE ExternFolder* ExternFolderRegistry::create(StringView desc) {
    // Make directory
    String folder_path =
        make_unique_file_name(Path.join(Workspace.path, "data/extern"), desc);
    FSResult fs_result = FileSystem.make_dirs(folder_path);
    if (!(fs_result == FSResult::OK || fs_result == FSResult::AlreadyExists)) {
        PLY_ASSERT(
            0); // Don't bother to handle gracefully; this library will be deleted soon
        return nullptr;
    }

    // Create ExternFolder object
    ExternFolder* folder = new ExternFolder;
    folder->path = std::move(folder_path);
    folder->desc = desc;
    this->folders.append(folder);
    return folder;
}

} // namespace build
} // namespace ply

#include "codegen/ExternFolderRegistry.inl" //%%
