#include <ply-build-repo/Module.h>

using namespace ply;
using namespace ply::build;

// ply instantiate reflect PLY_DLL
void inst_plyReflect(TargetInstantiatorArgs* args) {
    args->addSourceFiles("ply-reflect");
    args->setPrecompiledHeader("ply-reflect/Precomp.cpp", "ply-reflect/Core.h");
    args->addIncludeDir(Visibility::Public, ".");
    args->addTarget(Visibility::Public, "runtime");
}

// ply instantiate PlyDLL
void inst_plyDLL(TargetInstantiatorArgs* args) {
    args->buildTarget->targetType = BuildTargetType::DLL;
    args->addTarget(Visibility::Private, "runtime");
    args->addTarget(Visibility::Private, "reflect");
}
