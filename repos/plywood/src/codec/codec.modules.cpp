#include <ply-build-repo/Module.h>

using namespace ply;
using namespace ply::build;

// ply instantiate codec
void inst_ply_codec(TargetInstantiatorArgs* args) {
    args->addSourceFiles("ply-codec");
    args->addIncludeDir(Visibility::Public, ".");
    args->addTarget(Visibility::Public, "runtime");
    args->addTarget(Visibility::Public, "image");
    args->addTarget(Visibility::Public, "audio-primitives");
    args->addExtern(Visibility::Private, "libavcodec");
}

// ply extern plywood.libavcodec.macports
ExternResult extern_libavcodec_macports(ExternCommand cmd, ExternProviderArgs* args) {
    if (args->providerArgs) {
        return {ExternResult::BadArgs, ""};
    }
    PackageManager* pkgMan = HostTools::get()->getMacPorts();
    if (!pkgMan) {
        return {ExternResult::MissingPackageManager, {}};
    }

    ExternResult er = {pkgMan->isPackageInstalled("ffmpeg")
                           ? ExternResult::Installed
                           : ExternResult::SupportedButNotInstalled,
                       ""};
    if (cmd == ExternCommand::Status) {
        return er;
    } else if (cmd == ExternCommand::Install) {
        if (er.code == ExternResult::Installed) {
            return er;
        }
        return {pkgMan->installPackage("ffmpeg") ? ExternResult::Installed
                                                 : ExternResult::InstallFailed,
                ""};
    } else if (cmd == ExternCommand::Instantiate) {
        if (er.code != ExternResult::Installed) {
            return er;
        }

        String prefix = pkgMan->getInstallPrefix("ffmpeg");
        args->dep->includeDirs.append(NativePath::join(prefix, "include"));
        args->dep->libs.append(NativePath::join(prefix, "lib/libavcodec.dylib"));
        args->dep->libs.append(NativePath::join(prefix, "lib/libavutil.dylib"));
        args->dep->libs.append(NativePath::join(prefix, "lib/libswresample.dylib"));
        args->dep->libs.append(NativePath::join(prefix, "lib/libswscale.dylib"));
        args->dep->libs.append(NativePath::join(prefix, "lib/libavformat.dylib"));
        return {ExternResult::Instantiated, ""};
    }
    PLY_ASSERT(0);
    return {ExternResult::Unknown, ""};
}

// ply extern plywood.libavcodec.apt
ExternResult extern_libavcodec_apt(ExternCommand cmd, ExternProviderArgs* args) {
    if (args->providerArgs) {
        return {ExternResult::BadArgs, ""};
    }
    PackageManager* pkgMan = HostTools::get()->getApt();
    if (!pkgMan) {
        return {ExternResult::MissingPackageManager, {}};
    }

    ExternResult er = {pkgMan->isPackageInstalled("libavcodec-dev")
                           ? ExternResult::Installed
                           : ExternResult::SupportedButNotInstalled,
                       ""};
    if (cmd == ExternCommand::Status) {
        return er;
    } else if (cmd == ExternCommand::Install) {
        if (er.code == ExternResult::Installed) {
            return er;
        }
        return {pkgMan->installPackage("libavcodec-dev") ? ExternResult::Installed
                                                   : ExternResult::InstallFailed,
                ""};
    } else if (cmd == ExternCommand::Instantiate) {
        if (er.code != ExternResult::Installed) {
            return er;
        }
        args->dep->libs.append("-lavcodec");
        args->dep->libs.append("-lavutil");
        args->dep->libs.append("-lavformat");
        args->dep->libs.append("-lswresample");
        args->dep->libs.append("-lswscale");
        return {ExternResult::Instantiated, ""};
    }
    PLY_ASSERT(0);
    return {ExternResult::Unknown, ""};
}

// ply extern plywood.libavcodec.prebuilt
ExternResult extern_libavcodec_prebuilt(ExternCommand cmd, ExternProviderArgs* args) {
    // Toolchain filters
    if (args->toolchain->targetPlatform.name != "windows") {
        return {ExternResult::UnsupportedToolchain, "Target platform must be 'windows'"};
    }
    if (findItem(ArrayView<const StringView>{"x86", "x64"}, args->toolchain->arch) < 0) {
        return {ExternResult::UnsupportedToolchain, "Target arch must be 'x86' or 'x64'"};
    }
    if (args->providerArgs) {
        return {ExternResult::BadArgs, ""};
    }

    StringView libArch = (args->toolchain->arch == "x86" ? "win32" : "win64");
    StringView version = "4.2.2";
    StringView avcodecVersion = "avcodec-58";
    StringView avutilVersion = "avutil-56";
    StringView avformatVersion = "avformat-58";
    StringView swresampleVersion = "swresample-3";
    StringView swscaleVersion = "swscale-5";

    // Handle Command
    Tuple<ExternResult, ExternFolder*> er = args->findExistingExternFolder(args->toolchain->arch);
    if (cmd == ExternCommand::Status) {
        return er.first;
    } else if (cmd == ExternCommand::Install) {
        if (er.first.code != ExternResult::SupportedButNotInstalled) {
            return er.first;
        }
        ExternFolder* externFolder = args->createExternFolder(args->toolchain->arch);
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
