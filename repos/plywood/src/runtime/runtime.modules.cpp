#include <ply-build-repo/Module.h>

// ply instantiate runtime
void inst_plyRuntime(TargetInstantiatorArgs* args) {
    args->buildTarget->dynamicLinkPrefix = "PLY_DLL";
    args->addIncludeDir(Visibility::Public, ".");
    args->addTarget(Visibility::Public, "platform");
    args->addSourceFilesWhenImported("ply-runtime", {"memory/Heap.cpp"});
    args->addSourceFiles("ply-runtime");
    args->setPrecompiledHeader("ply-runtime/Precomp.cpp", "ply-runtime/Precomp.h");
    args->buildTarget->libs.append("${PLY_RUNTIME_DEPENDENCIES}");
}
