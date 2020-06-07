#include <ply-build-repo/Module.h>

// ply instantiate math
void inst_ply_math(TargetInstantiatorArgs* args) {
    args->addSourceFiles("math/ply-math");
    args->addIncludeDir(Visibility::Public, "math");
    args->addTarget(Visibility::Public, "platform");
}

// ply instantiate math-serial
void inst_ply_mathSerial(TargetInstantiatorArgs* args) {
    args->addSourceFiles("serial/ply-math-serial");
    args->addIncludeDir(Visibility::Public, "serial");
    args->addTarget(Visibility::Public, "math");
    args->addTarget(Visibility::Public, "reflect");
}
