/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-math/Core.h>
#include <ply-math/Vector.h>

namespace ply {

PLY_NO_INLINE Float3 Float3::normalized() const {
    return *this / length();
}

PLY_NO_INLINE Float3 Float3::safeNormalized(const Float3& fallback, float epsilon) const {
    float L = length2();
    if (L < 1e-20f)
        return epsilon;
    return *this / sqrtf(L);
}    

PLY_NO_INLINE Float3 clamp(const Float3& v, const Float3& mins, const Float3& maxs) {
    return {clamp(v.x, mins.x, maxs.x), clamp(v.y, mins.y, maxs.y), clamp(v.z, mins.z, maxs.z)};
}

PLY_NO_INLINE Float3 cross(const Float3& a, const Float3& b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

PLY_NO_INLINE Float3 pow(const Float3& a, const Float3& b) {
    return {powf(a.x, b.x), powf(a.y, b.y), powf(a.z, b.z)};
}

PLY_NO_INLINE Float3 quantizeNearest(const Float3& value, float spacing) {
    return {quantizeNearest(value.x, spacing), quantizeNearest(value.y, spacing),
            quantizeNearest(value.z, spacing)};
}

PLY_NO_INLINE bool isQuantized(const Float3& value, float spacing) {
    return quantizeNearest(value, spacing) == value;
}

PLY_NO_INLINE Float3 quantizeDown(const Float3& value, float spacing) {
    return {quantizeDown(value.x, spacing), quantizeDown(value.y, spacing),
            quantizeDown(value.z, spacing)};
}

PLY_NO_INLINE Float3 quantizeUp(const Float3& value, float spacing) {
    return {quantizeUp(value.x, spacing), quantizeUp(value.y, spacing),
            quantizeUp(value.z, spacing)};
}

PLY_NO_INLINE Float4 Float4::normalized() const {
    return *this / length();
}

PLY_NO_INLINE Float4 Float4::safeNormalized(const Float4& fallback, float epsilon) const {
    float L = length2();
    if (L < 1e-20f)
        return epsilon;
    return *this / sqrtf(L);
}    

} // namespace ply
