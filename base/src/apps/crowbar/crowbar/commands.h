/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                    ┃
┃    ╱   ╱╲    Plywood C++ Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/           ┃
┃    └──┴┴┴┘                                  ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/

#pragma once
#include "core.h"

void tidy_repo(StringView repoPath, StringView clangFormatPath, const TextFormat& tff);
void print_bigfont(StringView text);
void do_codegen();
void command_open(BuildFolder_t* bf);
Tuple<s32, String> generateCMakeProject(StringView cmakeListsFolder,
                                        const CMakeGeneratorOptions& generatorOpts,
                                        StringView config);
void generate(BuildFolder_t* build_folder);
void write_bootstrap(BuildFolder_t* build_folder, u32 configIndex);
bool create_build_folder(StringView name);
Array<String> get_build_folders();
bool set_build_folder(StringView name);
