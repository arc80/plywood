/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <Core.h>
#include <WorkspaceSettings.h>
#include <pylon/Parse.h>
#include <pylon/Write.h>
#include <pylon-reflect/Import.h>
#include <pylon-reflect/Export.h>

namespace ply {

PLY_NO_INLINE bool WorkspaceSettings::load() {
    String path = this->getPath();
    String strContents = FileSystem::native()->loadTextAutodetect(path).first;
    if (FileSystem::native()->lastResult() != FSResult::OK)
        return true;

    auto aRoot = pylon::Parser{}.parse(path, strContents).root;
    if (!aRoot->isValid())
        return false;

    importInto(AnyObject::bind(this), aRoot);
    if (!this->sourceNewLines) {
        this->sourceNewLines =
            (TextFormat::platformPreference().newLine == TextFormat::NewLine::CRLF ? "crlf" : "lf");
    }
    return true;
}

PLY_NO_INLINE bool WorkspaceSettings::save() const {
    auto aRoot = pylon::exportObj(AnyObject::bind(this));
    String strContents = pylon::toString(aRoot);
    // FIXME: makeDirsAndSaveTextIfDifferent should write to temp file with atomic rename
    FSResult result = FileSystem::native()->makeDirsAndSaveTextIfDifferent(
        getPath(), strContents, this->getSourceTextFormat());
    if (result != FSResult::OK && result != FSResult::Unchanged) {
        StdErr::text() << "Error: Can't save workspace settings.\n";
        return false;
    }
    return true;
}

PLY_NO_INLINE TextFormat WorkspaceSettings::getSourceTextFormat() const {
    TextFormat tff = TextFormat::platformPreference();
    if (this->sourceNewLines == "crlf") {
        tff.newLine = TextFormat::NewLine::CRLF;
    } else if (this->sourceNewLines == "lf") {
        tff.newLine = TextFormat::NewLine::LF;
    }
    return tff;
}

} // namespace ply

#include "codegen/WorkspaceSettings.inl" //%%
