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
    String strContents = FileSystem::native()->loadTextAutodetect(getPath()).first;
    if (FileSystem::native()->lastResult() != FSResult::OK)
        return true;

    auto aRoot = pylon::Parser{}.parse(strContents).root;
    if (!aRoot.isValid())
        return false;

    importInto(TypedPtr::bind(this), aRoot);
    return true;
}

PLY_NO_INLINE bool WorkspaceSettings::save() const {
    auto aRoot = pylon::exportObj(TypedPtr::bind(this));
    String strContents = pylon::toString(aRoot);
    // FIXME: makeDirsAndSaveTextIfDifferent should write to temp file with atomic rename
    FSResult result = FileSystem::native()->makeDirsAndSaveTextIfDifferent(
        getPath(), strContents, TextFormat::platformPreference());
    if (result != FSResult::OK && result != FSResult::Unchanged) {
        StdErr::createStringWriter() << "Error: Can't save workspace settings.\n";
        return false;
    }
    return true;
}

} // namespace ply

#include "codegen/WorkspaceSettings.inl" //%%
