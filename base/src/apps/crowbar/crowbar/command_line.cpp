/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include "core.h"
#include "command_line.h"
#include <ply-runtime/algorithm/Find.h>
#include <ply-runtime/Error.h>

bool prefixMatch(StringView input, StringView cmd, u32 minUnits) {
    if (input.numBytes < minUnits)
        return false;
    return cmd.startsWith(input);
}

StringView CommandLine::readToken() {
    while (this->index < this->args.numItems()) {
        StringView arg = this->args[this->index];
        this->index++;
        if (arg.startsWith("-")) {
            this->skippedOpts.append(arg);
        } else {
            return arg;
        }
    }
    return {};
}

StringView CommandLine::checkForSkippedOpt(const Functor<bool(StringView)>& matcher) {
    s32 i = find(this->skippedOpts, matcher);
    if (i >= 0) {
        StringView matched = this->skippedOpts[i];
        this->skippedOpts.erase(i);
        return matched;
    }
    return {};
}

void CommandLine::finalize() {
    PLY_ASSERT(!this->finalized);
    if (this->skippedOpts.numItems() > 0) {
        Error.log("Unrecognized option '{}'\n", this->skippedOpts[0]);
        exit(1);
    }
    this->finalized = true;
}

void ensureTerminated(CommandLine* cl) {
    StringView token = cl->readToken();
    if (!token.isEmpty()) {
        Error.log("Unexpected token \"{}\"", token);
        exit(1);
    }
}
