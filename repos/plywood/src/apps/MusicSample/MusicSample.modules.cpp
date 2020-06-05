#include <ply-build-repo/Module.h>

using namespace ply;
using namespace ply::build;

// ply instantiate MusicSample
void inst_MusicSample(TargetInstantiatorArgs* args) {
    args->buildTarget->targetType = BuildTargetType::EXE;
    args->addSourceFiles(".", false);
    args->addIncludeDir(Visibility::Private, ".");
    args->addTarget(Visibility::Private, "codec");
}
