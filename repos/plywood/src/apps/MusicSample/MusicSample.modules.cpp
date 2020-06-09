#include <ply-build-repo/Module.h>

// [ply module="MusicSample"]
void module_MusicSample(ModuleArgs* args) {
    args->buildTarget->targetType = BuildTargetType::EXE;
    args->addSourceFiles(".", false);
    args->addIncludeDir(Visibility::Private, ".");
    args->addTarget(Visibility::Private, "codec");
}
