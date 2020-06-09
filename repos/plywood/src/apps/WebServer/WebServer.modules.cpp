#include <ply-build-repo/Module.h>

// [ply module="WebServer"]
void module_WebServer(ModuleArgs* args) {
    args->buildTarget->targetType = BuildTargetType::EXE;
    args->addSourceFiles(".");
    args->addIncludeDir(Visibility::Private, ".");
    args->addIncludeDir(Visibility::Public, NativePath::join(args->projInst->env->buildFolderPath,
                                                             "codegen/WebServer"));
    args->addTarget(Visibility::Private, "pylon-reflect");
    args->addTarget(Visibility::Private, "web-common");
    args->addTarget(Visibility::Private, "web-serve-docs");
    args->addTarget(Visibility::Private, "web-documentation");

    if (args->projInst->env->isGenerating) {
        String configFile = String::format(
            R"(#define WEBSERVER_DEFAULT_PORT {}
#define WEBSERVER_DEFAULT_DOC_DIR "{}"
)",
            8080, fmt::EscapedString{NativePath::join(PLY_WORKSPACE_FOLDER, "data/docsite")});
        FileSystem::native()->makeDirsAndSaveTextIfDifferent(
            NativePath::join(args->projInst->env->buildFolderPath,
                             "codegen/WebServer/WebServer/Config.h"),
            configFile, TextFormat::platformPreference());
    }
}
