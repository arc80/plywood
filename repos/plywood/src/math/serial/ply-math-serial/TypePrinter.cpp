/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-math/Core.h>
#include <ply-math-serial/TypePrinter.h>

namespace ply {
namespace fmt {

PLY_NO_INLINE void TypePrinter<Float2>::print(StringWriter* sw, const Float2& v) {
    sw->format("{{{}, {}}}", v.x, v.y);
}

PLY_NO_INLINE void TypePrinter<Float3>::print(StringWriter* sw, const Float3& v) {
    sw->format("{{{}, {}, {}}}", v.x, v.y, v.z);
}

PLY_NO_INLINE void TypePrinter<Float4>::print(StringWriter* sw, const Float4& v) {
    sw->format("{{{}, {}, {}, {}}}", v.x, v.y, v.z, v.w);
}

} // namespace fmt
} // namespace ply
