/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-repo/Module.h>

// [ply module="codec"]
void module_ply_codec(ModuleArgs* args) {
    args->addSourceFiles("ply-codec");
    args->addIncludeDir(Visibility::Public, ".");
    args->addTarget(Visibility::Public, "runtime");
    args->addTarget(Visibility::Public, "image");
    args->addTarget(Visibility::Public, "audio-primitives");
    args->addExtern(Visibility::Private, "libavcodec");
}

// [ply extern="libavcodec" provider="macports"]
ExternResult extern_libavcodec_macports(ExternCommand cmd, ExternProviderArgs* args) {
    PackageProvider prov{
        PackageProvider::MacPorts, "ffmpeg", [&](StringView prefix) {
            args->dep->includeDirs.append(NativePath::join(prefix, "include"));
            args->dep->libs.append(NativePath::join(prefix, "lib/libavcodec.dylib"));
            args->dep->libs.append(NativePath::join(prefix, "lib/libavutil.dylib"));
            args->dep->libs.append(NativePath::join(prefix, "lib/libswresample.dylib"));
            args->dep->libs.append(NativePath::join(prefix, "lib/libswscale.dylib"));
            args->dep->libs.append(NativePath::join(prefix, "lib/libavformat.dylib"));
        }};
    return prov.handle(cmd, args);
}

// [ply extern="libavcodec" provider="apt"]
ExternResult extern_libavcodec_apt(ExternCommand cmd, ExternProviderArgs* args) {
    PackageProvider prov{PackageProvider::Apt, "libavcodec-dev", [&](StringView prefix) {
                             args->dep->libs.append("-lavcodec");
                             args->dep->libs.append("-lavutil");
                             args->dep->libs.append("-lavformat");
                             args->dep->libs.append("-lswresample");
                             args->dep->libs.append("-lswscale");
                         }};
    return prov.handle(cmd, args);
}

// [ply extern="libavcodec" provider="prebuilt"]
ExternResult extern_libavcodec_prebuilt(ExternCommand cmd, ExternProviderArgs* args) {
    // Toolchain filters
    if (args->toolchain->get("targetPlatform")->text() != "windows") {
        return {ExternResult::UnsupportedToolchain, "Target platform must be 'windows'"};
    }
    StringView arch = args->toolchain->get("arch")->text();
    if (arch != "x64") {
        return {ExternResult::UnsupportedToolchain, "Target arch must be x64'"};
    }
    if (args->providerArgs) {
        return {ExternResult::BadArgs, ""};
    }

    StringView url =
        "https://github.com/BtbN/FFmpeg-Builds/releases/download/autobuild-2020-10-26-12-33/"
        "ffmpeg-n4.3.1-20-g8a2acdc6da-win64-lgpl-shared-4.3.zip";
    String archiveName = PosixPath::split(url).second;
    Array<StringView> libVersions = {"avcodec-58",  "avutil-56",    "avfilter-7",
                                     "avformat-58", "swresample-3", "swscale-5"};

    // Handle Command
    Tuple<ExternResult, ExternFolder*> er = args->findExistingExternFolder(arch);
    if (cmd == ExternCommand::Status) {
        return er.first;
    } else if (cmd == ExternCommand::Install) {
        if (er.first.code != ExternResult::SupportedButNotInstalled) {
            return er.first;
        }
        ExternFolder* externFolder = args->createExternFolder(arch);
        String archivePath = NativePath::join(externFolder->path, archiveName);

        if (!downloadFile(archivePath, url)) {
            return {ExternResult::InstallFailed, String::format("Error downloading '{}'", url)};
        }
        if (!extractFile(archivePath)) {
            return {ExternResult::InstallFailed,
                    String::format("Error extracting '{}'", archivePath)};
        }
        FileSystem::native()->deleteFile(archivePath);
        externFolder->success = true;
        externFolder->save();
        return {ExternResult::Installed, ""};
    } else if (cmd == ExternCommand::Instantiate) {
        if (er.first.code != ExternResult::Installed) {
            return er.first;
        }
        String installPath =
            NativePath::join(er.second->path, PosixPath::splitExt(archiveName).first);

        args->dep->includeDirs.append(NativePath::join(installPath, "include"));
        for (StringView libVersion : libVersions) {
            args->dep->libs.append(
                NativePath::join(installPath, "lib", libVersion.splitByte('-')[0] + ".lib"));
            args->dep->dlls.append(NativePath::join(installPath, "bin", libVersion + ".dll"));
        }
        return {ExternResult::Instantiated, ""};
    }
    PLY_ASSERT(0);
    return {ExternResult::Unknown, ""};
}
