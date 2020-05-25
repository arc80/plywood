/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-math/Core.h>
#include <ply-math/AxisVector.h>

namespace ply {

const Float2x2 ReflectXform[(u32) Reflection::Count] = {
    {{1, 0}, {0, 1}},  {{0, 1}, {-1, 0}}, {{-1, 0}, {0, -1}}, {{0, -1}, {1, 0}},
    {{-1, 0}, {0, 1}}, {{0, 1}, {1, 0}},  {{1, 0}, {0, -1}},  {{0, -1}, {-1, 0}}};

} // namespace ply
