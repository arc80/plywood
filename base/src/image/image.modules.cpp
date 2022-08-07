/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-repo/Module.h>

// [ply module="image"]
void module_image(ModuleArgs* args) {
    args->addSourceFiles("image/image");
    args->addIncludeDir(Visibility::Public, "image");
    args->addTarget(Visibility::Public, "runtime");
    args->addTarget(Visibility::Public, "math");
}

// [ply module="image-reflect"]
void module_image_reflect(ModuleArgs* args) {
    args->addSourceFiles("reflect/image-reflect");
    args->addIncludeDir(Visibility::Public, "reflect");
    args->addTarget(Visibility::Public, "reflect");
    args->addTarget(Visibility::Public, "image");
}

// [ply module="image-cairo"]
void module_image_cairo(ModuleArgs* args) {
    // FIXME: Automatically detect HeaderOnly based on source file extensions
    args->buildTarget->targetType = BuildTargetType::HeaderOnly;
    args->addSourceFiles("cairo/image-cairo");
    args->addIncludeDir(Visibility::Public, "cairo");
    args->addTarget(Visibility::Public, "image");
    args->addExtern(Visibility::Public, "cairo");
}

// [ply module="image-png"]
void module_image_png(ModuleArgs* args) {
    // FIXME: Automatically detect HeaderOnly based on source file extensions
    args->addSourceFiles("png/image-png");
    args->addIncludeDir(Visibility::Public, "png");
    args->addTarget(Visibility::Public, "image");
    args->addTarget(Visibility::Public, "runtime");
    args->addExtern(Visibility::Private, "libpng");
}

// [ply extern="cairo" provider="macports"]
ExternResult extern_cairo_macports(ExternCommand cmd, ExternProviderArgs* args) {
    PackageProvider prov{PackageProvider::MacPorts, "cairo", [&](StringView prefix) {
                             args->dep->includeDirs.append(
                                 NativePath::join(prefix, "include/cairo"));
                             args->dep->libs.append(NativePath::join(prefix, "lib/libcairo.dylib"));
                         }};
    return prov.handle(cmd, args);
}

// [ply extern="cairo" provider="apt"]
ExternResult extern_cairo_apt(ExternCommand cmd, ExternProviderArgs* args) {
    PackageProvider prov{PackageProvider::Apt, "cairo", [&](StringView prefix) {
                             args->dep->includeDirs.append("/usr/include/cairo");
                             args->dep->libs.append("-lcairo");
                         }};
    return prov.handle(cmd, args);
}

// [ply extern="cairo" provider="prebuilt"]
ExternResult extern_cairo_prebuilt(ExternCommand cmd, ExternProviderArgs* args) {
    // Toolchain filters
    if (args->toolchain->get("targetPlatform")->text() != "windows") {
        return {ExternResult::UnsupportedToolchain, "Target platform must be 'windows'"};
    }
    StringView arch = args->toolchain->get("arch")->text();
    if (find<StringView>({"x86", "x64"}, arch) < 0) {
        return {ExternResult::UnsupportedToolchain, "Target arch must be 'x86' or 'x64'"};
    }
    if (args->providerArgs) {
        return {ExternResult::BadArgs, ""};
    }

    StringView version = "1.17.2";
    String archiveName = String::format("cairo-windows-{}", version);

    // Handle Command
    Tuple<ExternResult, ExternFolder*> er = args->findExistingExternFolder(arch);
    if (cmd == ExternCommand::Status) {
        return er.first;
    } else if (cmd == ExternCommand::Install) {
        if (er.first.code != ExternResult::SupportedButNotInstalled) {
            return er.first;
        }
        ExternFolder* externFolder = args->createExternFolder(arch);
        String archivePath = NativePath::join(externFolder->path, archiveName + ".zip");
        String url =
            String::format("https://github.com/preshing/cairo-windows/releases/download/{}/{}.zip",
                           version, archiveName);
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
        StringView platformFolder = arch;
        args->dep->libs.append(NativePath::join(installFolder, "lib", platformFolder, "cairo.lib"));
        args->dep->dlls.append(NativePath::join(installFolder, "lib", platformFolder, "cairo.dll"));
        return {ExternResult::Instantiated, ""};
    }
    PLY_ASSERT(0);
    return {ExternResult::Unknown, ""};
}

// [ply extern="libpng" provider="builtFromSource"]
ExternResult extern_libpng_builtFromSource(ExternCommand cmd, ExternProviderArgs* args) {
    if (args->toolchain->get("targetPlatform")->text() != "windows") {
        return {ExternResult::UnsupportedToolchain, "Target platform must be 'windows'"};
    }
    StringView arch = args->toolchain->get("arch")->text();
    if (find<StringView>({"x86", "x64"}, arch) < 0) {
        return {ExternResult::UnsupportedToolchain, "Target arch must be 'x86' or 'x64'"};
    }
    if (args->providerArgs) {
        return {ExternResult::BadArgs, ""};
    }

    // Handle Command
    Tuple<ExternResult, ExternFolder*> er = args->findExistingExternFolder({});
    if (cmd == ExternCommand::Status) {
        return er.first;
    } else if (cmd == ExternCommand::Install) {
        if (er.first.code != ExternResult::SupportedButNotInstalled) {
            return er.first;
        }
        ExternFolder* externFolder = args->createExternFolder({});
        externFolder->success = true;
        externFolder->save();
        return {ExternResult::Installed, ""};
    } else if (cmd == ExternCommand::Instantiate) {
        // FIXME: Detect build configuration (compiler, target platform) and adapt settings
        String installFolder = er.second->path;
        args->dep->includeDirs.append(NativePath::join(installFolder, "libpng"));
        args->dep->includeDirs.append(NativePath::join(installFolder, "zlib"));
        StringView archFolder;
        if (arch == "x64") {
            archFolder = "x64";
        }
        args->dep->libs.append(NativePath::join(installFolder, "libpng/projects/vstudio",
                                                archFolder, "Release Library/libpng16.lib"));
        args->dep->libs.append(NativePath::join(installFolder, "libpng/projects/vstudio",
                                                archFolder, "Release Library/zlib.lib"));
        return {ExternResult::Instantiated, ""};
    }
    PLY_ASSERT(0);
    return {ExternResult::Unknown, ""};
}
