/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <Core.h>
#include <Workspace.h>
#include <pylon/Parse.h>
#include <pylon/Write.h>
#include <pylon-reflect/Import.h>
#include <pylon-reflect/Export.h>

Workspace_ Workspace;

PLY_NO_INLINE void Workspace_::load() {
    String dir = FileSystem::native()->getWorkingDirectory();
    static const StringView fileName = "workspace-settings.pylon";

    // Search each parent directory for workspace-settings.pylon:
    while (true) {
        this->path = NativePath::join(this->path, fileName);
        if (FileSystem::native()->exists(this->path) == ExistsResult::File)
            break;
        String nextDir = NativePath::split(dir).first;
        if (dir == nextDir) {
            // We've reached the topmost directory.
            Error.log(String::format("Can't locate {}", fileName));
            exit(1);
        }
        dir = nextDir;
    }

    String contents = FileSystem::native()->loadTextAutodetect(this->path).first;
    if (FileSystem::native()->lastResult() != FSResult::OK) {
        Error.log(String::format("Can't read {}", this->path));
        exit(1);
    }

    auto aRoot = pylon::Parser{}.parse(this->path, contents).root;
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
        Error.log(String::format("Can't save workspace settings to {}", this->path));
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

#include "codegen/Workspace.inl" //%%
