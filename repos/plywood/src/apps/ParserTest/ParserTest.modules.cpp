#include <ply-build-repo/Module.h>

using namespace ply;
using namespace ply::build;

// ply instantiate ParserTest
void inst_ParserTest(TargetInstantiatorArgs* args) {
    args->buildTarget->targetType = BuildTargetType::EXE;
    args->addSourceFiles(".", false);
    args->addIncludeDir(Visibility::Private, ".");
    args->addTarget(Visibility::Private, "cpp");
    args->addTarget(Visibility::Private, "pylon-reflect");
}

