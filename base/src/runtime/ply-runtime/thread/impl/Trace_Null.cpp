/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>

#if !PLY_DLL_IMPORTING

#include <memory>
#include <ply-runtime/thread/impl/Trace_Null.h>

namespace ply {

Trace_Null Trace_Null::Instance;

} // namespace ply

#endif // !PLY_DLL_IMPORTING
