#include <ply-build-repo/Module.h>

// ply instantiate image
void inst_image(TargetInstantiatorArgs* args) {
    args->addSourceFiles("image/image");
    args->addIncludeDir(Visibility::Public, "image");
    args->addTarget(Visibility::Public, "runtime");
    args->addTarget(Visibility::Public, "math");
}

// ply instantiate image-reflect
void inst_image_reflect(TargetInstantiatorArgs* args) {
    args->addSourceFiles("reflect/image-reflect");
    args->addIncludeDir(Visibility::Public, "reflect");
    args->addTarget(Visibility::Public, "reflect");
    args->addTarget(Visibility::Public, "image");
}

// ply instantiate image-cairo
void inst_image_cairo(TargetInstantiatorArgs* args) {
    // FIXME: Automatically detect HeaderOnly based on source file extensions
    args->buildTarget->targetType = BuildTargetType::HeaderOnly;
    args->addSourceFiles("cairo/image-cairo");
    args->addIncludeDir(Visibility::Public, "cairo");
    args->addTarget(Visibility::Public, "image");
    args->addExtern(Visibility::Public, "cairo");
}

// ply instantiate image-png
void inst_image_png(TargetInstantiatorArgs* args) {
    // FIXME: Automatically detect HeaderOnly based on source file extensions
    args->addSourceFiles("png/image-png");
    args->addIncludeDir(Visibility::Public, "png");
    args->addTarget(Visibility::Public, "image");
    args->addTarget(Visibility::Public, "runtime");
    args->addExtern(Visibility::Private, "libpng");
}

// ply extern plywood.cairo.macports
ExternResult extern_cairo_macports(ExternCommand cmd, ExternProviderArgs* args) {
    if (args->providerArgs) {
        return {ExternResult::BadArgs, ""};
    }
    PackageManager* pkgMan = HostTools::get()->getMacPorts();
    if (!pkgMan) {
        return {ExternResult::MissingPackageManager, {}};
    }

    ExternResult er = {pkgMan->isPackageInstalled("cairo")
                           ? ExternResult::Installed
                           : ExternResult::SupportedButNotInstalled,
                       ""};
    if (cmd == ExternCommand::Status) {
        return er;
    } else if (cmd == ExternCommand::Install) {
        if (er.code == ExternResult::Installed) {
            return er;
        }
        return {pkgMan->installPackage("cairo") ? ExternResult::Installed
                                                   : ExternResult::InstallFailed,
                ""};
    } else if (cmd == ExternCommand::Instantiate) {
        if (er.code != ExternResult::Installed) {
            return er;
        }
        String prefix = pkgMan->getInstallPrefix("cairo");
        args->dep->includeDirs.append(NativePath::join(prefix, "include/cairo"));
        args->dep->libs.append(NativePath::join(prefix, "lib/libcairo.dylib"));
        return {ExternResult::Instantiated, ""};
    }
    PLY_ASSERT(0);
    return {ExternResult::Unknown, ""};
}

// ply extern plywood.cairo.apt
ExternResult extern_cairo_apt(ExternCommand cmd, ExternProviderArgs* args) {
    if (args->providerArgs) {
        return {ExternResult::BadArgs, ""};
    }
    PackageManager* pkgMan = HostTools::get()->getApt();
    if (!pkgMan) {
        return {ExternResult::MissingPackageManager, {}};
    }

    ExternResult er = {pkgMan->isPackageInstalled("libcairo2-dev")
                           ? ExternResult::Installed
                           : ExternResult::SupportedButNotInstalled,
                       ""};
    if (cmd == ExternCommand::Status) {
        return er;
    } else if (cmd == ExternCommand::Install) {
        if (er.code == ExternResult::Installed) {
            return er;
        }
        return {pkgMan->installPackage("libcairo2-dev") ? ExternResult::Installed
                                                   : ExternResult::InstallFailed,
                ""};
    } else if (cmd == ExternCommand::Instantiate) {
        if (er.code != ExternResult::Installed) {
            return er;
        }
        args->dep->includeDirs.append("/usr/include/cairo");
        args->dep->libs.append("-lcairo");
        return {ExternResult::Instantiated, ""};
    }
    PLY_ASSERT(0);
    return {ExternResult::Unknown, ""};
}


// ply extern plywood.cairo.prebuilt
ExternResult extern_cairo_prebuilt(ExternCommand cmd, ExternProviderArgs* args) {
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

    StringView version = "1.15.12";
    String archiveName = String::format("cairo-windows-{}", version);

    // Handle Command
    Tuple<ExternResult, ExternFolder*> er = args->findExistingExternFolder(args->toolchain->arch);
    if (cmd == ExternCommand::Status) {
        return er.first;
    } else if (cmd == ExternCommand::Install) {
        if (er.first.code != ExternResult::SupportedButNotInstalled) {
            return er.first;
        }
        ExternFolder* externFolder = args->createExternFolder(args->toolchain->arch);
        String archivePath = NativePath::join(externFolder->path, archiveName + ".zip");
        String url = String::format("https://github.com/preshing/cairo-windows/releases/download/{}/{}.zip", version, archiveName);
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
        String installFolder = NativePath::join(er.second->path, archiveName);
        args->dep->includeDirs.append(NativePath::join(installFolder, "include"));
        StringView platformFolder = args->toolchain->arch;
        args->dep->libs.append(NativePath::join(installFolder, "lib", platformFolder, "cairo.lib"));
        args->dep->dlls.append(NativePath::join(installFolder, "lib", platformFolder, "cairo.dll"));
        return {ExternResult::Instantiated, ""};
    }
    PLY_ASSERT(0);
    return {ExternResult::Unknown, ""};
}

// ply extern plywood.libpng.builtFromSource
ExternResult extern_libpng_builtFromSource(ExternCommand cmd, ExternProviderArgs* args) {
    PLY_ASSERT(0); // FIXME
    return {ExternResult::Unknown, ""};
    /*
    if (cmd == ExternCommand::Instantiate) {
        // FIXME: Detect build configuration (compiler, target platform) and adapt settings
        String installFolder = NativePath::join(PLY_WORKSPACE_FOLDER, "data/extern");
        args->dep->includeDirs.append(NativePath::join(installFolder, "libpng"));
        args->dep->includeDirs.append(NativePath::join(installFolder, "zlib"));
        args->dep->libs.append(NativePath::join(
            installFolder, "libpng/projects/vstudio/Release Library/libpng16.lib"));
        args->dep->libs.append(
            NativePath::join(installFolder, "libpng/projects/vstudio/Release Library/zlib.lib"));
        return true;
    }
    return false;
    */
}
