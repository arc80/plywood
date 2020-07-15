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
    if (findItem(ArrayView<const StringView>{"x86", "x64"}, arch) < 0) {
        return {ExternResult::UnsupportedToolchain, "Target arch must be 'x86' or 'x64'"};
    }
    if (args->providerArgs) {
        return {ExternResult::BadArgs, ""};
    }

    StringView libArch = (arch == "x86" ? "win32" : "win64");
    StringView version = "4.2.2";
    StringView avcodecVersion = "avcodec-58";
    StringView avutilVersion = "avutil-56";
    StringView avformatVersion = "avformat-58";
    StringView swresampleVersion = "swresample-3";
    StringView swscaleVersion = "swscale-5";

    // Handle Command
    Tuple<ExternResult, ExternFolder*> er = args->findExistingExternFolder(arch);
    if (cmd == ExternCommand::Status) {
        return er.first;
    } else if (cmd == ExternCommand::Install) {
        if (er.first.code != ExternResult::SupportedButNotInstalled) {
            return er.first;
        }
        ExternFolder* externFolder = args->createExternFolder(arch);
        for (StringView archiveType : ArrayView<const StringView>{"shared", "dev"}) {
            String archiveName =
                String::format("ffmpeg-{}-{}-{}.zip", version, libArch, archiveType);
            String archivePath = NativePath::join(externFolder->path, archiveName + ".zip");
            String url = String::format("https://ffmpeg.zeranoe.com/builds/{}/{}/{}", libArch,
                                        archiveType, archiveName);
            if (!downloadFile(archivePath, url)) {
                return {ExternResult::InstallFailed, String::format("Error downloading '{}'", url)};
            }
            if (!extractFile(archivePath)) {
                return {ExternResult::InstallFailed,
                        String::format("Error extracting '{}'", archivePath)};
            }
            FileSystem::native()->deleteFile(archivePath);
        }
        externFolder->success = true;
        externFolder->save();
        return {ExternResult::Installed, ""};
    } else if (cmd == ExternCommand::Instantiate) {
        if (er.first.code != ExternResult::Installed) {
            return er.first;
        }
        args->dep->includeDirs.append(NativePath::join(
            er.second->path, String::format("ffmpeg-{}-{}-dev/include", version, libArch)));
        args->dep->libs.append(NativePath::join(
            er.second->path, String::format("ffmpeg-{}-{}-dev/lib/avcodec.lib", version, libArch)));
        args->dep->libs.append(NativePath::join(
            er.second->path, String::format("ffmpeg-{}-{}-dev/lib/avutil.lib", version, libArch)));
        args->dep->libs.append(
            NativePath::join(er.second->path, String::format("ffmpeg-{}-{}-dev/lib/swresample.lib",
                                                             version, libArch)));
        args->dep->libs.append(NativePath::join(
            er.second->path, String::format("ffmpeg-{}-{}-dev/lib/swscale.lib", version, libArch)));
        args->dep->libs.append(
            NativePath::join(er.second->path, String::format("ffmpeg-{}-{}-dev/lib/avformat.lib",
                                                             version, libArch)));
        args->dep->dlls.append(
            NativePath::join(er.second->path, String::format("ffmpeg-{}-{}-shared/bin/{}.dll",
                                                             version, libArch, avcodecVersion)));
        args->dep->dlls.append(
            NativePath::join(er.second->path, String::format("ffmpeg-{}-{}-shared/bin/{}.dll",
                                                             version, libArch, avutilVersion)));
        args->dep->dlls.append(
            NativePath::join(er.second->path, String::format("ffmpeg-{}-{}-shared/bin/{}.dll",
                                                             version, libArch, avformatVersion)));
        args->dep->dlls.append(
            NativePath::join(er.second->path, String::format("ffmpeg-{}-{}-shared/bin/{}.dll",
                                                             version, libArch, swresampleVersion)));
        args->dep->dlls.append(
            NativePath::join(er.second->path, String::format("ffmpeg-{}-{}-shared/bin/{}.dll",
                                                             version, libArch, swscaleVersion)));
        return {ExternResult::Instantiated, ""};
    }
    PLY_ASSERT(0);
    return {ExternResult::Unknown, ""};
}
