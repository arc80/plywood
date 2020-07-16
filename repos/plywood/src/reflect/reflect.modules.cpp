/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-repo/Module.h>

// [ply module="reflect"]
void module_plyReflect(ModuleArgs* args) {
    args->buildTarget->dynamicLinkPrefix = "PLY_DLL";
    args->addSourceFiles("ply-reflect");
    args->setPrecompiledHeader("ply-reflect/Precomp.cpp", "ply-reflect/Core.h");
    args->addIncludeDir(Visibility::Public, ".");
    args->addTarget(Visibility::Public, "runtime");
}

