#include <ply-build-repo/Module.h>

// ply instantiate WebCooker
void inst_WebCooker(TargetInstantiatorArgs* args) {
    args->buildTarget->targetType = BuildTargetType::EXE;
    args->addSourceFiles(".");
    args->addIncludeDir(Visibility::Private, ".");
    args->addTarget(Visibility::Private, "pylon-reflect");
    args->addTarget(Visibility::Private, "web-sass");
    args->addTarget(Visibility::Private, "web-documentation");
    args->addTarget(Visibility::Private, "cook");
    args->addTarget(Visibility::Private, "web-cook-docs");
}

