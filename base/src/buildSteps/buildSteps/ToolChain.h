/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <buildSteps/Core.h>

namespace buildSteps {

struct ToolChain;

struct CompileOpts {
    Array<String> compileFlags;
    Array<String> linkFlags;
};

Owned<ToolChain> getMSVC();
Owned<ToolChain> getGCC();
void translateGenericOption(ToolChain* tc, CompileOpts* copts, StringView genericKey, StringView genericValue);
void destroy(ToolChain* tc);

} // namespace buildSteps
