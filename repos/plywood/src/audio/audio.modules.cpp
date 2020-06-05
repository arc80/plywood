#include <ply-build-repo/Module.h>

using namespace ply;
using namespace ply::build;

// ply instantiate audio-primitives
void inst_audioPrimitives(TargetInstantiatorArgs* args) {
    args->addSourceFiles("primitives/audio-primitives");
    args->addIncludeDir(Visibility::Public, "primitives");
    args->addTarget(Visibility::Public, "reflect");
}
