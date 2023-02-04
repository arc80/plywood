/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <web-sass/Core.h>
#include <web-sass/Sass.h>

namespace ply {
namespace web {

SassResult convertSassToStylesheet(const char* path) {
    SassResult result;
    result.context = sass_make_file_context(path);

    Sass_Options* options = sass_file_context_get_options(result.context);
    sass_option_set_precision(options, 1);
    // sass_option_set_source_comments(options, true);
    sass_file_context_set_options(result.context, options);

    Sass_Compiler* compiler = sass_make_file_compiler(result.context);
    sass_compiler_parse(compiler);
    sass_compiler_execute(compiler);
    sass_delete_compiler(compiler);

    return result;
}

} // namespace web
} // namespace ply
