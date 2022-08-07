/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-build-common/Core.h>

namespace ply {
namespace build {

PLY_BUILD_ENTRY bool downloadFile(StringView localFile, StringView sourceURL);
PLY_BUILD_ENTRY bool extractFile(StringView archivePath);

} // namespace build
} // namespace ply
