/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-web-cook-docs/Core.h>
#include <ply-cook/CookJob.h>

namespace ply {
namespace docs {

void cook_CopyStatic(cook::CookResult* cook_result, AnyObject) {
    PLY_ASSERT(!cook_result->job->id.desc.is_empty());

    // Create destination folder(s) if missing
    String dst_path =
        Path.join(Workspace.path, "data/docsite/static", cook_result->job->id.desc);
    FSResult r = FileSystem.make_dirs(Path.split(dst_path).first);
    if (r != FSResult::OK && r != FSResult::AlreadyExists) {
        // FIXME: add reason from r
        cook_result->add_error(String::format("unable to create '{}'", dst_path));
        return;
    }

    // Create Dependency on source file
    String src_path = Path.join(Workspace.path, "repos/plywood/src/web/theme",
                                cook_result->job->id.desc);
    cook::CookResult::FileDepScope fd_scope =
        cook_result->create_file_dependency(src_path);
    PLY_UNUSED(fd_scope);

    // Open source file
    Owned<InPipe> in_pipe = FileSystem.open_pipe_for_read(src_path);
    if (!in_pipe) {
        // FIXME: add reason from last_result()
        cook_result->add_error(String::format("can't open '{}'", src_path));
        return;
    }

    // Allocate temporary storage
    String buf = String::allocate(32768);

    // Open destination file
    // FIXME: Copy to temporary file first, then rename it
    Owned<OutPipe> out_pipe = FileSystem.open_pipe_for_write(dst_path);
    if (!out_pipe) {
        // FIXME: add reason from last_result()
        cook_result->add_error(String::format("unable to create '{}'", dst_path));
        return;
    }

    // Copy in chunks
    for (;;) {
        u32 num_bytes = in_pipe->read_some({buf.bytes, buf.num_bytes});
        // FIXME: Distinguish failed read from EOF
        if (num_bytes == 0)
            break;
        out_pipe->write(buf.left(num_bytes));
    }
}

cook::CookJobType CookJobType_CopyStatic = {
    "copyStatic",
    get_type_descriptor<cook::CookResult>(),
    nullptr,
    cook_CopyStatic,
};

} // namespace docs
} // namespace ply
