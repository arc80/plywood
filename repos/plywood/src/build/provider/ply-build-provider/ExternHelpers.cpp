/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-common/Core.h>
#include <ply-build-provider/ExternHelpers.h>

namespace ply {
namespace build {

bool downloadFile(StringView localFile, StringView sourceURL) {
    PLY_ASSERT(NativePath::isAbsolute(localFile));

    // Prepare CMake script to download file
    String script = String::format(
        R"(file(DOWNLOAD "{}" "{}" STATUS statusVar)
message("${{statusVar}}")
)",
        fmt::EscapedString{sourceURL}, fmt::EscapedString{PosixPath::from<NativePath>(localFile)});

    // Save CMake script
    String scriptFile = localFile + ".cmake";
    FSResult rc = FileSystem::native()->makeDirsAndSaveTextIfDifferent(
        scriptFile, script, TextFormat::platformPreference());
    if (rc != FSResult::OK)
        return false;
    PLY_ON_SCOPE_EXIT({ FileSystem::native()->deleteFile(scriptFile); });

    // Run CMake
    Owned<Subprocess> sub =
        Subprocess::exec(PLY_CMAKE_PATH, {"-P", scriptFile}, {}, Subprocess::Output::openMerged());
    if (!sub)
        return false;

    // Get result
    String output = StringReader{sub->readFromStdOut.borrow()}.readRemainingContents();
    u32 exitCode = sub->join();
    if (exitCode != 0)
        return false;
    if (!output.startsWith("0;"))
        return false;

    return true;
}

bool downloadFile(StringView localFile, ArrayView<const StringView> sourceURLs) {
    for (StringView sourceURL : sourceURLs) {
        if (downloadFile(localFile, sourceURL))
            return true;
    }
    return false;
}

bool extractFile(StringView archivePath) {
    auto splitPath = NativePath::split(archivePath);
    if (archivePath.endsWith(".msi")) {
#if PLY_TARGET_WIN32
        // FIXME: Should check for availability of msiexec on the host first (just as we'd check for
        // package managers like Conan)
        Owned<Subprocess> sub = Subprocess::execArgStr(
            "msiexec",
            String::format("/a {} /quiet TARGETDIR={}", fmt::CmdLineArg_WinCrt{archivePath},
                           fmt::CmdLineArg_WinCrt{archivePath.shortenedBy(4)}),
            splitPath.first, Subprocess::Output::ignore());
        u32 code = sub->join();
        return code == 0;
#else
        PLY_ASSERT(0);
        return false;
#endif        
    } else {
        Owned<Subprocess> sub = Subprocess::exec(PLY_CMAKE_PATH, {"-E", "tar", "zxf", archivePath},
                                                 splitPath.first, Subprocess::Output::ignore());
        u32 code = sub->join();
        return code == 0;
    }
}

} // namespace build
} // namespace ply
