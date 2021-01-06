/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-math/Core.h>
#include <ply-math/QuatPos.h>

namespace ply {

PLY_NO_INLINE QuatPos QuatPos::inverted() const {
    Quaternion qi = quat.inverted();
    return {qi, qi * -pos};
}

PLY_NO_INLINE QuatPos QuatPos::identity() {
    return {{0, 0, 0, 1}, {0, 0, 0}};
}

PLY_NO_INLINE QuatPos QuatPos::makeTranslation(const Float3& pos) {
    return {{0, 0, 0, 1}, pos};
}

PLY_NO_INLINE QuatPos QuatPos::makeRotation(const Float3& unitAxis, float radians) {
    return {Quaternion::fromAxisAngle(unitAxis, radians), {0, 0, 0}};
}

PLY_NO_INLINE QuatPos QuatPos::fromOrtho(const Float3x4& m) {
    return {Quaternion::fromOrtho(m.asFloat3x3()), m[3]};
}

PLY_NO_INLINE QuatPos QuatPos::fromOrtho(const Float4x4& m) {
    return {Quaternion::fromOrtho(m), m[3].asFloat3()};
}

} // namespace ply
