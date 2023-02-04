/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include "core.h"
#include <ply-build-repo/BuildFolder.h>

void tidy_source();
void print_bigfont(StringView text);
void print_smallbox(StringView text);
void do_codegen();
void command_open(build::BuildFolder_t* bf);
Tuple<s32, String> generateCMakeProject(StringView cmakeListsFolder,
                                        const build::CMakeGeneratorOptions& generatorOpts,
                                        StringView config);
void generate(build::BuildFolder_t* build_folder);
void write_bootstrap(build::BuildFolder_t* build_folder, u32 configIndex);
bool create_build_folder(StringView name);
Array<String> get_build_folders();
bool set_build_folder(StringView name);
