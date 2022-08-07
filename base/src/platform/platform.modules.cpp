/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-repo/Module.h>

// [ply module="platform"]
void module_plyPlatform(ModuleArgs* args) {
    args->buildTarget->dynamicLinkPrefix = "PLY_DLL";
    args->buildTarget->targetType = BuildTargetType::HeaderOnly;
    args->addSourceFiles("ply-platform");
    args->addIncludeDir(Visibility::Public, ".");
    args->addIncludeDir(Visibility::Public, NativePath::join(args->projInst->env->buildFolderPath,
                                                             "codegen/ply-platform"));

    if (args->projInst->env->isGenerating) {
        String configFile = String::format(
            R"(#define PLY_PREFER_CPP11 0
#define PLY_WITH_EXCEPTIONS 0
#define PLY_REPLACE_OPERATOR_NEW 1
#define PLY_USE_DLMALLOC 1
#define PLY_DLMALLOC_DEBUG_CHECKS 0
#define PLY_DLMALLOC_FAST_STATS 0

// Avoid degraded performance caused by Mutex_Win32 (FIXME: Make this the default?):
#define PLY_IMPL_MUTEX_PATH "impl/Mutex_CPP11.h"
#define PLY_IMPL_MUTEX_TYPE ply::Mutex_CPP11
#define PLY_IMPL_CONDITIONVARIABLE_PATH "impl/ConditionVariable_CPP11.h"
#define PLY_IMPL_CONDITIONVARIABLE_TYPE ply::ConditionVariable_CPP11

#define PLY_WORKSPACE_FOLDER "{}"
#define PLY_BUILD_FOLDER "{}"
#define PLY_CMAKE_PATH "{}"
#define WITH_AUDIO 1
)",
            fmt::EscapedString{PLY_WORKSPACE_FOLDER},
            fmt::EscapedString{args->projInst->env->buildFolderPath},
            fmt::EscapedString{PLY_CMAKE_PATH});
        FileSystem::native()->makeDirsAndSaveTextIfDifferent(
            NativePath::join(args->projInst->env->buildFolderPath,
                             "codegen/ply-platform/ply-platform/Config.h"),
            configFile, TextFormat::platformPreference());
    }
}
