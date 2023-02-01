/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-repo/Core.h>
#include <ply-build-repo/Workspace.h>
#include <pylon/Parse.h>
#include <pylon/Write.h>
#include <pylon-reflect/Import.h>
#include <pylon-reflect/Export.h>

namespace ply {

Workspace_t Workspace;

PLY_NO_INLINE void Workspace_t::load() {
    this->path = FileSystem.getWorkingDirectory();
    static const StringView fileName = "workspace-settings.pylon";
    String settingsPath;

    // Search each parent directory for workspace-settings.pylon:
    while (true) {
        settingsPath = Path.join(this->path, fileName);
        if (FileSystem.exists(settingsPath) == ExistsResult::File)
            break;
        String nextDir = Path.split(this->path).first;
        if (this->path == nextDir) {
            // We've reached the topmost directory.
            Error.log("Can't locate {}", fileName);
            exit(1);
        }
        this->path = nextDir;
    }

    String contents = FileSystem.loadTextAutodetect(settingsPath);
    if (FileSystem.lastResult() != FSResult::OK) {
        Error.log("Can't read {}", settingsPath);
        exit(1);
    }

    auto aRoot = pylon::Parser{}.parse(settingsPath, contents).root;
    if (!aRoot->isValid()) {
        exit(1);
    }

    importInto(AnyObject::bind(this), aRoot);
    if (!this->sourceNewLines) {
        this->sourceNewLines =
            (TextFormat::default_utf8().newLine == TextFormat::NewLine::CRLF ? "crlf"
                                                                             : "lf");
    }
}

PLY_NO_INLINE bool Workspace_t::save() const {
    static const StringView fileName = "workspace-settings.pylon";
    auto aRoot = pylon::exportObj(AnyObject::bind(this));
    String contents = pylon::toString(aRoot);
    FSResult result = FileSystem.makeDirsAndSaveTextIfDifferent(
        Path.join(this->path, fileName), contents, this->getSourceTextFormat());
    if (result != FSResult::OK && result != FSResult::Unchanged) {
        Error.log("Can't save workspace settings to {}", this->path);
        return false;
    }
    return true;
}

PLY_NO_INLINE TextFormat Workspace_t::getSourceTextFormat() const {
    TextFormat tff = TextFormat::default_utf8();
    if (this->sourceNewLines == "crlf") {
        tff.newLine = TextFormat::NewLine::CRLF;
    } else if (this->sourceNewLines == "lf") {
        tff.newLine = TextFormat::NewLine::LF;
    }
    return tff;
}

#include "codegen/Workspace.inl" //%%

} // namespace ply