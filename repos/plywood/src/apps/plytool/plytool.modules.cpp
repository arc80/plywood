#include <ply-build-repo/Module.h>

using namespace ply;
using namespace ply::build;

// ply instantiate plytool
void inst_plytool(TargetInstantiatorArgs* args) {
    args->buildTarget->targetType = BuildTargetType::EXE;
    args->addSourceFiles("plytool");
    args->addIncludeDir(Visibility::Private, "plytool");
    args->addTarget(Visibility::Private, "build-folder");
    args->addTarget(Visibility::Private, "cpp");
    args->addTarget(Visibility::Private, "pylon-reflect");
    args->addTarget(Visibility::Private, "plytool-client");
}

// ply instantiate plytool-client
void inst_plytool_client(TargetInstantiatorArgs* args) {
    args->addSourceFiles("plytool-client/plytool-client");
    args->addIncludeDir(Visibility::Public, "plytool-client");
    args->addTarget(Visibility::Public, "reflect");
}
