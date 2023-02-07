/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-math/Core.h>
#include <ply-math/Vector.h>

namespace ply {

//------------------------------------------------
// Float2
//------------------------------------------------
Float2 Float2::normalized() const {
    return *this / length();
}

Float2 Float2::safe_normalized(const Float2& fallback, float epsilon) const {
    float L = length2();
    if (L < epsilon)
        return fallback;
    return *this / sqrtf(L);
}

Rect rect_from_fov(float fov_y, float aspect) {
    float half_tan_y = tanf(fov_y / 2);
    return expand(Rect{{0, 0}}, {half_tan_y * aspect, half_tan_y});
}

//------------------------------------------------
// Float3
//------------------------------------------------
Float3 Float3::normalized() const {
    return *this / length();
}

Float3 Float3::safe_normalized(const Float3& fallback, float epsilon) const {
    float L = length2();
    if (L < epsilon)
        return fallback;
    return *this / sqrtf(L);
}

Float3 clamp(const Float3& v, const Float3& mins, const Float3& maxs) {
    return {clamp(v.x, mins.x, maxs.x), clamp(v.y, mins.y, maxs.y),
            clamp(v.z, mins.z, maxs.z)};
}

Float3 cross(const Float3& a, const Float3& b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

Float3 pow(const Float3& a, const Float3& b) {
    return {powf(a.x, b.x), powf(a.y, b.y), powf(a.z, b.z)};
}

//------------------------------------------------
// Float4
//------------------------------------------------
Float4 Float4::normalized() const {
    return *this / length();
}

Float4 Float4::safe_normalized(const Float4& fallback, float epsilon) const {
    float L = length2();
    if (L < epsilon)
        return fallback;
    return *this / sqrtf(L);
}

Float4 pow(const Float4& a, const Float4& b) {
    return {powf(a.x, b.x), powf(a.y, b.y), powf(a.z, b.z), powf(a.w, b.w)};
}

} // namespace ply
