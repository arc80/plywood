/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <Core.h>
#include <ply-build-folder/BuildFolder.h>

struct CommandLine {
    Array<StringView> args;
    u32 index = 1;
    Array<StringView> skippedOpts;
    bool finalized = false;

    CommandLine(int argc, char* argv[])
        : args{ArrayView<const char*>{(const char**) argv, (u32) argc}} {
    }
    ~CommandLine() {
        PLY_ASSERT(this->finalized);
    }

    StringView readToken();
    StringView checkForSkippedOpt(const Functor<bool(StringView)>& matcher);
    PLY_INLINE bool checkForSkippedOpt(StringView argToMatch) {
        return !this->checkForSkippedOpt(
            [argToMatch](StringView arg) -> bool { return arg == argToMatch; });
    }
    void finalize();
};

struct CrowbarCommandEnv {
    CommandLine* cl = nullptr;
};

bool prefixMatch(StringView input, StringView cmd, u32 minUnits = 2);
void ensureTerminated(CommandLine* cl);
