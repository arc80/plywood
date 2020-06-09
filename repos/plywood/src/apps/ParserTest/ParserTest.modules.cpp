#include <ply-build-repo/Module.h>

// [ply module="ParserTest"]
void module_ParserTest(ModuleArgs* args) {
    args->buildTarget->targetType = BuildTargetType::EXE;
    args->addSourceFiles(".", false);
    args->addIncludeDir(Visibility::Private, ".");
    args->addTarget(Visibility::Private, "cpp");
    args->addTarget(Visibility::Private, "pylon-reflect");
}

