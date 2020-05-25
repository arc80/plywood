/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-common/Core.h>
#include <ply-build-provider/HostTools.h>

namespace ply {
namespace build {

Owned<HostTools> HostTools::instance_;

PLY_NO_INLINE Owned<HostTools> HostTools::create() {
    PLY_ASSERT(!instance_);
    return new HostTools;
}

} // namespace build
} // namespace ply
