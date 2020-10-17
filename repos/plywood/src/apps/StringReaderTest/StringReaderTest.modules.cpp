/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-repo/Module.h>

// [ply module="StringReaderTest"]
void module_StringReaderTest(ModuleArgs* args) {
    args->buildTarget->targetType = BuildTargetType::EXE;
    args->addSourceFiles(".", false);
    args->addIncludeDir(Visibility::Private, ".");
    args->addTarget(Visibility::Private, "runtime");
}

