/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <web-sass/Core.h>
#include <sass.h>

namespace ply {
namespace web {

struct SassResult {
    Sass_File_Context* context = nullptr;

    SassResult() = default;
    SassResult(SassResult&& other) {
        context = other.context;
        other.context = nullptr;
    }
    ~SassResult() {
        if (context) {
            sass_delete_file_context(context);
        }
    }
};

SassResult convertSassToStylesheet(const char* path);

} // namespace web
} // namespace ply
