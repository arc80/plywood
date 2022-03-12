/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-web-cook-docs/Core.h>
#include <ply-cook/CookJob.h>

namespace ply {
namespace docs {

void cook_CopyStatic(cook::CookResult* cookResult, TypedPtr) {
    PLY_ASSERT(!cookResult->job->id.desc.isEmpty());

    // Create destination folder(s) if missing
    String dstPath =
        NativePath::join(PLY_WORKSPACE_FOLDER, "data/docsite/static", cookResult->job->id.desc);
    FSResult r = FileSystem::native()->makeDirs(NativePath::split(dstPath).first);
    if (r != FSResult::OK && r != FSResult::AlreadyExists) {
        // FIXME: add reason from r
        cookResult->addError(String::format("unable to create '{}'", dstPath));
        return;
    }

    // Create Dependency on source file
    String srcPath = NativePath::join(PLY_WORKSPACE_FOLDER, "repos/plywood/src/web/theme",
                                      cookResult->job->id.desc);
    cook::CookResult::FileDepScope fdScope = cookResult->createFileDependency(srcPath);
    PLY_UNUSED(fdScope);

    // Open source file
    Owned<InPipe> inPipe = FileSystem::native()->openPipeForRead(srcPath);
    if (!inPipe) {
        // FIXME: add reason from lastResult()
        cookResult->addError(String::format("can't open '{}'", srcPath));
        return;
    }

    // Allocate temporary storage
    String buf = String::allocate(32768);

    // Open destination file
    // FIXME: Copy to temporary file first, then rename it
    Owned<OutPipe> outPipe = FileSystem::native()->openPipeForWrite(dstPath);
    if (!outPipe) {
        // FIXME: add reason from lastResult()
        cookResult->addError(String::format("unable to create '{}'", dstPath));
        return;
    }

    // Copy in chunks
    for (;;) {
        u32 numBytes = inPipe->readSome({buf.bytes, buf.numBytes});
        // FIXME: Distinguish failed read from EOF
        if (numBytes == 0)
            break;
        outPipe->write(buf.left(numBytes));
    }
}

cook::CookJobType CookJobType_CopyStatic = {
    "copyStatic",
    getTypeDescriptor<cook::CookResult>(),
    nullptr,
    cook_CopyStatic,
};

} // namespace docs
} // namespace ply
