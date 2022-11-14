/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <buildSteps/ToolChain.h>

namespace buildSteps {

void translateOptionMSVC(ToolChain* tc, CompileOpts* copts, const buildSteps::ToolchainOpt& opt) {
    PLY_ASSERT(opt.type == buildSteps::ToolchainOpt::Type::Generic);
    if (opt.key == "optimization") {
        if (opt.value == "size") {
            copts->compileFlags.append("/Os");
        } else if (opt.value == "speed") {
            copts->compileFlags.append("/O2");
        } else {
            // FIXME: Handle gracefully
            PLY_ASSERT(0);
        }
    } else {
        // FIXME: Handle gracefully
        PLY_ASSERT(0);
    }
}

Owned<ToolChain> createToolChainMSVC() {
    auto tc = Owned<ToolChain>::create();
    tc->translateOption = {translateOptionMSVC, tc.get()};
    return tc;
}

} // namespace buildSteps
