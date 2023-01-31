/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                   ┃
┃    ╱   ╱╲    Plywood Multimedia Toolkit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/          ┃
┃    └──┴┴┴┘                                 ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/

#include "core.h"
#include "command_line.h"
#include <ply-runtime/Algorithm.h>

CommandLine::CommandLine(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        StringView arg = argv[i];
        if (arg.startsWith("-")) {
            StringView key = arg.subStr(arg.startsWith("--") ? 2 : 1);
            s32 e = key.findByte('=');
            if (e > 0) {
                this->options.append({key.left(e), key.subStr(e + 1)});
            } else {
                this->options.append({key});
            }
        } else {
            this->args.append(arg);
        }
    }
}

CommandLine::~CommandLine() {
    PLY_ASSERT(this->error_checked);
}

bool prefix_match(StringView input, StringView cmd, u32 min_units) {
    if (input.numBytes < min_units)
        return false;
    return cmd.startsWith(input);
}

StringView CommandLine::next_arg() {
    if (this->arg_index < this->args.numItems()) {
        return this->args[arg_index++];
    }
    return {};
}

bool CommandLine::find_option(StringView key, StringView* out_value) {
    s32 i = find(this->options, [&](const Option& o) { return o.key == key; });
    if (i >= 0) {
        this->options[i].checked = true;
        if (out_value) {
            *out_value = this->options[i].value;
        }
        return true;
    }
    return false;
}

void CommandLine::check_for_unused_args() {
    PLY_ASSERT(!this->error_checked);
    if (this->arg_index < this->args.numItems()) {
        Error.log("Unused argument '{}'\n", this->args[this->arg_index]);
        exit(1);
    }
    for (const Option& option : this->options) {
        if (!option.checked) {
            Error.log("Unrecognized option '{}'\n", option.key);
            exit(1);
        }
    }
    this->error_checked = true;
}
