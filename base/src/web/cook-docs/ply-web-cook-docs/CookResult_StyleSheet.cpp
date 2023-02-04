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

void StyleSheet_cook(cook::CookResult* cookResult, AnyObject) {
    PLY_ASSERT(cookResult->job->id.desc.isEmpty());

    // Add dependency
    String filePath =
        Path.join(Workspace.path, "repos/plywood/src/web/theme/style.scss");
    cook::CookResult::FileDepScope fdScope = cookResult->createFileDependency(filePath);
    PLY_UNUSED(fdScope);

    // FIXME: Get names of any included files, too!
    web::SassResult result = web::convertSassToStylesheet(filePath.withNullTerminator().bytes);
    int error_status = sass_context_get_error_status((Sass_Context*) result.context);
    if (error_status != 0) {
        StringView errorMessage = sass_context_get_error_message((Sass_Context*) result.context);
        cookResult->addError(errorMessage);
        return;
    }

    // Save result
    // FIXME: Implement strategy to delete orphaned CSS files
    String cssPath = Path.join(Workspace.path, "data/docsite/static/stylesheet.css");
    StringView cssText = sass_context_get_output_string((Sass_Context*) result.context);
    FileSystem.makeDirsAndSaveTextIfDifferent(cssPath, cssText, TextFormat::unixUTF8());
}

cook::CookJobType CookJobType_StyleSheetID = {
    "stylesheet",
    getTypeDescriptor<cook::CookResult>(),
    nullptr,
    StyleSheet_cook,
};

} // namespace docs
} // namespace ply
