/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include "core.h"
#include "workspace.h"
#include <pylon/Parse.h>
#include <pylon/Write.h>
#include <pylon-reflect/Import.h>
#include <pylon-reflect/Export.h>

Workspace_ Workspace;

PLY_NO_INLINE void Workspace_::load() {
    this->path = FileSystem::native()->getWorkingDirectory();
    static const StringView fileName = "workspace-settings.pylon";
    String settingsPath;

    // Search each parent directory for workspace-settings.pylon:
    while (true) {
        settingsPath = NativePath::join(this->path, fileName);
        if (FileSystem::native()->exists(settingsPath) == ExistsResult::File)
            break;
        String nextDir = NativePath::split(this->path).first;
        if (this->path == nextDir) {
            // We've reached the topmost directory.
            Error.log("Can't locate {}", fileName);
            exit(1);
        }
        this->path = nextDir;
    }

    String contents = FileSystem::native()->loadTextAutodetect(settingsPath).first;
    if (FileSystem::native()->lastResult() != FSResult::OK) {
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
            (TextFormat::platformPreference().newLine == TextFormat::NewLine::CRLF ? "crlf" : "lf");
    }
}

PLY_NO_INLINE void Workspace_::save() const {
    auto aRoot = pylon::exportObj(AnyObject::bind(this));
    String contents = pylon::toString(aRoot);
    // FIXME: makeDirsAndSaveTextIfDifferent should write to temp file with atomic rename
    FSResult result = FileSystem::native()->makeDirsAndSaveTextIfDifferent(
        this->path, contents, this->getSourceTextFormat());
    if (result != FSResult::OK && result != FSResult::Unchanged) {
        Error.log("Can't save workspace settings to {}", this->path);
    }
}

PLY_NO_INLINE TextFormat Workspace_::getSourceTextFormat() const {
    TextFormat tff = TextFormat::platformPreference();
    if (this->sourceNewLines == "crlf") {
        tff.newLine = TextFormat::NewLine::CRLF;
    } else if (this->sourceNewLines == "lf") {
        tff.newLine = TextFormat::NewLine::LF;
    }
    return tff;
}

#include "codegen/workspace.inl" //%%
