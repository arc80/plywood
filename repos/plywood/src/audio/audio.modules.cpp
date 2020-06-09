#include <ply-build-repo/Module.h>

// [ply module="audio-primitives"]
void module_audioPrimitives(ModuleArgs* args) {
    args->addSourceFiles("primitives/audio-primitives");
    args->addIncludeDir(Visibility::Public, "primitives");
    args->addTarget(Visibility::Public, "reflect");
}
