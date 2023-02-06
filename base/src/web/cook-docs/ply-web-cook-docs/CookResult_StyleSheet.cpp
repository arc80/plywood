/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-web-cook-docs/Core.h>
#include <ply-cook/CookJob.h>
#include <web-sass/Sass.h>

namespace ply {
namespace docs {

void StyleSheet_cook(cook::CookResult* cook_result, AnyObject) {
    PLY_ASSERT(cook_result->job->id.desc.is_empty());

    // Add dependency
    String file_path =
        Path.join(Workspace.path, "repos/plywood/src/web/theme/style.scss");
    cook::CookResult::FileDepScope fd_scope =
        cook_result->create_file_dependency(file_path);
    PLY_UNUSED(fd_scope);

    // FIXME: Get names of any included files, too!
    web::SassResult result =
        web::convert_sass_to_stylesheet(file_path.with_null_terminator().bytes);
    int error_status = sass_context_get_error_status((Sass_Context*) result.context);
    if (error_status != 0) {
        StringView error_message =
            sass_context_get_error_message((Sass_Context*) result.context);
        cook_result->add_error(error_message);
        return;
    }

    // Save result
    // FIXME: Implement strategy to delete orphaned CSS files
    String css_path = Path.join(Workspace.path, "data/docsite/static/stylesheet.css");
    StringView css_text =
        sass_context_get_output_string((Sass_Context*) result.context);
    FileSystem.make_dirs_and_save_text_if_different(css_path, css_text,
                                                    TextFormat::unix_utf8());
}

cook::CookJobType CookJobType_StyleSheetID = {
    "stylesheet",
    get_type_descriptor<cook::CookResult>(),
    nullptr,
    StyleSheet_cook,
};

} // namespace docs
} // namespace ply
