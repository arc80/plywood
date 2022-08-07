/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-common/Core.h>
#include <ply-build-target/TargetError.h>

namespace ply {
namespace build {

thread_local Functor<void(StringView)> TargetError::callback_;

} // namespace build
} // namespace ply
