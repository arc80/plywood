/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <Core.h>
#include <ConsoleUtils.h>
#include <ply-runtime/algorithm/Find.h>

namespace ply {

bool prefixMatch(StringView input, StringView cmd, u32 minUnits) {
    if (input.numBytes < minUnits)
        return false;
    return cmd.startsWith(input);
}

void fatalError(StringView msg) {
    OutStream stdErr = StdErr::text();
    stdErr << "error: " << msg;
    if (!msg.endsWith("\n")) {
        stdErr << '\n';
    }
    stdErr.flushMem();
    exit(1);
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

StringView CommandLine::checkForSkippedOpt(const LambdaView<bool(StringView)>& matcher) {
    s32 i = find(this->skippedOpts.view(), matcher);
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
        fatalError(String::format("Unrecognized option '{}'\n", this->skippedOpts[0]));
    }
    this->finalized = true;
}

void ensureTerminated(CommandLine* cl) {
    StringView token = cl->readToken();
    if (!token.isEmpty()) {
        fatalError(String::format("Unexpected token \"{}\"", token));
    }
}

void printCommands(OutStream* outs, const CommandList& commands) {
    *outs << "Available commands:\n\n";

    for (auto& d : commands) {
        *outs << d.name;
        for (s32 i = 0; i < 20 - (s32) d.name.numBytes; ++i) {
            *outs << ' ';
        }
        *outs << d.description << "\n";
    }
}

void printUsage(OutStream* outs, CommandList commands) {
    *outs << "Usage: plytool <command>\n\n";

    if (commands.numItems > 0) {
        printCommands(outs, commands);
    }

    *outs << "\n";
    outs->flush();
}

void printUsage(OutStream* outs, StringView command, CommandList commands) {
    *outs << "Usage: plytool " << command << " [<command>]\n";

    if (commands.numItems > 0) {
        printCommands(outs, commands);
    }

    *outs << "\n";
    outs->flush();
}

} // namespace ply
