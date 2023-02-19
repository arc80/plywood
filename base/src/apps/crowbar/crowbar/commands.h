﻿/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
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
Tuple<s32, String>
generate_cmake_project(StringView cmake_lists_folder,
                       const build::CMakeGeneratorOptions& generator_opts,
                       StringView config);
void generate(build::BuildFolder_t* build_folder);
void do_prebuild_steps(build::BuildFolder_t* build_folder, StringView config_name);
void write_bootstrap(build::BuildFolder_t* build_folder, u32 config_index);
bool create_build_folder(StringView name);
Array<String> get_build_folders();
bool set_build_folder(StringView name);
