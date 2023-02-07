/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-build-repo/Core.h>
#include <ply-build-repo/Workspace.h>
#include <pylon/Parse.h>
#include <pylon/Write.h>
#include <pylon-reflect/Import.h>
#include <pylon-reflect/Export.h>

namespace ply {

Workspace_t Workspace;

void Workspace_t::load() {
    this->path = FileSystem.get_working_directory();
    static const StringView file_name = "workspace-settings.pylon";
    String settings_path;

    // Search each parent directory for workspace-settings.pylon:
    while (true) {
        settings_path = Path.join(this->path, file_name);
        if (FileSystem.exists(settings_path) == ExistsResult::File)
            break;
        String next_dir = Path.split(this->path).first;
        if (this->path == next_dir) {
            // We've reached the topmost directory.
            Error.log("Can't locate {}", file_name);
            exit(1);
        }
        this->path = next_dir;
    }

    String contents = FileSystem.load_text_autodetect(settings_path);
    if (FileSystem.last_result() != FSResult::OK) {
        Error.log("Can't read {}", settings_path);
        exit(1);
    }

    auto a_root = pylon::Parser{}.parse(settings_path, contents).root;
    if (!a_root->is_valid()) {
        exit(1);
    }

    import_into(AnyObject::bind(this), a_root);
    if (!this->source_new_lines) {
        this->source_new_lines =
            (TextFormat::default_utf8().new_line == TextFormat::NewLine::CRLF ? "crlf"
                                                                              : "lf");
    }
}

bool Workspace_t::save() const {
    static const StringView file_name = "workspace-settings.pylon";
    auto a_root = pylon::export_obj(AnyObject::bind(this));
    String contents = pylon::to_string(a_root);
    FSResult result = FileSystem.make_dirs_and_save_text_if_different(
        Path.join(this->path, file_name), contents, this->get_source_text_format());
    if (result != FSResult::OK && result != FSResult::Unchanged) {
        Error.log("Can't save workspace settings to {}", this->path);
        return false;
    }
    return true;
}

TextFormat Workspace_t::get_source_text_format() const {
    TextFormat tff = TextFormat::default_utf8();
    if (this->source_new_lines == "crlf") {
        tff.new_line = TextFormat::NewLine::CRLF;
    } else if (this->source_new_lines == "lf") {
        tff.new_line = TextFormat::NewLine::LF;
    }
    return tff;
}

#include "codegen/Workspace.inl" //%%

} // namespace ply