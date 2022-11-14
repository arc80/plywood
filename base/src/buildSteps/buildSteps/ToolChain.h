/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <buildSteps/Core.h>
#include <buildSteps/Project.h>

namespace buildSteps {

struct CompileOpts {
    Array<String> compileFlags;
    Array<String> linkFlags;
};

struct ToolChain;
struct ToolchainOpt;
typedef void TranslateOption(CompileOpts* copts, const ToolchainOpt& opt);

struct ToolChain {
    Functor<TranslateOption> translateOption;
    ~ToolChain() {}
};

Owned<ToolChain> createToolChainMSVC();
Owned<ToolChain> createToolChainGCC();

} // namespace buildSteps
