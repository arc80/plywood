/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <buildSteps/Project.h>

namespace ply {
namespace build2 {

void (*translate_toolchain_option)(CompilerSpecificOptions* copts, const Option& opt) = nullptr;

void translate_option_msvc(CompilerSpecificOptions* copts, const Option& opt) {
    PLY_ASSERT(opt.type == Option::Generic);
    if (opt.key == "optimization") {
        if (opt.value == "size") {
            copts->compile.append("/Os");
        } else if (opt.value == "speed") {
            copts->compile.append("/O2");
        } else {
            // FIXME: Handle gracefully
            PLY_ASSERT(0);
        }
    } else {
        // FIXME: Handle gracefully
        PLY_ASSERT(0);
    }
}

void init_toolchain_msvc() {
    translate_toolchain_option = translate_option_msvc;
}

} // namespace build
} // namespace ply

