/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-repo/Module.h>

// [ply module="runtime"]
void module_plyRuntime(ModuleArgs* args) {
    args->buildTarget->dynamicLinkPrefix = "PLY_DLL";
    args->addIncludeDir(Visibility::Public, ".");
    args->addTarget(Visibility::Public, "platform");
    args->addSourceFilesWhenImported("ply-runtime", {"memory/Heap.cpp"});
    args->addSourceFiles("ply-runtime");
    args->setPrecompiledHeader("ply-runtime/Precomp.cpp", "ply-runtime/Precomp.h");
    args->buildTarget->dep->libs.append("${PLY_RUNTIME_DEPENDENCIES}");
}

// [ply module="runtime-tests"]
void module_ply_runtimeTests(ModuleArgs* args) {
    args->buildTarget->targetType = BuildTargetType::ObjectLib;
    args->addSourceFiles("tests");
    args->addTarget(Visibility::Private, "runtime");
    args->addTarget(Visibility::Private, "test");
}
