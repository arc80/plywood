/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include "core.h"

struct CommandLine {
    struct Option {
        StringView key;
        StringView value;
        bool checked = false;
    };

    Array<StringView> args;
    u32 arg_index = 0;
    Array<Option> options;
    bool error_checked = false;

    CommandLine(int argc, char* argv[]);
    ~CommandLine();

    StringView next_arg();
    bool find_option(StringView key, StringView* out_value = nullptr);
    void check_for_unused_args();
};

bool prefix_match(StringView input, StringView cmd, u32 min_units = 2);
