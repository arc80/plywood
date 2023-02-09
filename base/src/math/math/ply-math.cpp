/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-math.h>

namespace ply {

//   ▄▄▄▄         ▄▄
//  ██  ██ ▄▄  ▄▄ ▄▄  ▄▄▄▄
//  ██▀▀██  ▀██▀  ██ ▀█▄▄▄
//  ██  ██ ▄█▀▀█▄ ██  ▄▄▄█▀
//

Axis cross(Axis va, Axis vb) {
    s32 a = s32(va);
    s32 b = s32(vb);
    s32 diff = b - (a & 6);
    PLY_ASSERT((diff & ~1) != 0); // must be perpendicular
    diff += (diff < 0) * 6;
    return Axis(6 ^ a ^ b ^ (diff >> 2));
}

s32 dot(Axis va, Axis vb) {
    s32 x = s32(va) ^ s32(vb);
    s32 mask = ((x & 6) != 0) - 1;
    s32 sgn = 1 - (x & 1) * 2;
    return sgn & mask;
}

//                        ▄▄▄▄
//  ▄▄   ▄▄  ▄▄▄▄   ▄▄▄▄ ▀▀  ██
//  ▀█▄ ▄█▀ ██▄▄██ ██     ▄█▀▀
//    ▀█▀   ▀█▄▄▄  ▀█▄▄▄ ██▄▄▄▄
//

vec2 vec2::normalized() const {
    return *this / length();
}

vec2 vec2::safe_normalized(const vec2& fallback, float epsilon) const {
    float L2 = square(*this);
    if (L2 < epsilon * epsilon)
        return fallback;
    return *this / sqrtf(L2);
}

Rect rect_from_fov(float fov_y, float aspect) {
    float half_tan_y = tanf(fov_y / 2);
    return inflate(Rect{{0, 0}}, {half_tan_y * aspect, half_tan_y});
}

vec2 round_up(const vec2& value, float spacing) {
    return {round_up(value.x, spacing), round_up(value.y, spacing)};
}

vec2 round_down(const vec2& value, float spacing) {
    return {round_down(value.x, spacing), round_down(value.y, spacing)};
}

vec2 round_nearest(const vec2& value, float spacing) {
    return {round_nearest(value.x, spacing), round_nearest(value.y, spacing)};
}

bool is_rounded(const vec2& value, float spacing) {
    return round_nearest(value, spacing) == value;
}

//                        ▄▄▄▄
//  ▄▄   ▄▄  ▄▄▄▄   ▄▄▄▄ ▀▀  ██
//  ▀█▄ ▄█▀ ██▄▄██ ██      ▀▀█▄
//    ▀█▀   ▀█▄▄▄  ▀█▄▄▄ ▀█▄▄█▀
//

vec3 vec3::normalized() const {
    return *this / length();
}

vec3 vec3::safe_normalized(const vec3& fallback, float epsilon) const {
    float L2 = square(*this);
    if (L2 < epsilon * epsilon)
        return fallback;
    return *this / sqrtf(L2);
}

vec3 clamp(const vec3& v, const vec3& mins, const vec3& maxs) {
    return {clamp(v.x, mins.x, maxs.x), clamp(v.y, mins.y, maxs.y),
            clamp(v.z, mins.z, maxs.z)};
}

vec3 cross(const vec3& a, const vec3& b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

vec3 pow(const vec3& a, const vec3& b) {
    return {powf(a.x, b.x), powf(a.y, b.y), powf(a.z, b.z)};
}

vec3 round_up(const vec3& value, float spacing) {
    return {round_up(value.x, spacing), round_up(value.y, spacing),
            round_up(value.z, spacing)};
}

vec3 round_down(const vec3& value, float spacing) {
    return {round_down(value.x, spacing), round_down(value.y, spacing),
            round_down(value.z, spacing)};
}

vec3 round_nearest(const vec3& value, float spacing) {
    return {round_nearest(value.x, spacing), round_nearest(value.y, spacing),
            round_nearest(value.z, spacing)};
}

bool is_rounded(const vec3& value, float spacing) {
    return round_nearest(value, spacing) == value;
}

//                          ▄▄▄
//  ▄▄   ▄▄  ▄▄▄▄   ▄▄▄▄  ▄█▀██
//  ▀█▄ ▄█▀ ██▄▄██ ██    ██▄▄██▄
//    ▀█▀   ▀█▄▄▄  ▀█▄▄▄     ██
//

vec4 vec4::normalized() const {
    return *this / length();
}

vec4 vec4::safe_normalized(const vec4& fallback, float epsilon) const {
    float L2 = square(*this);
    if (L2 < epsilon * epsilon)
        return fallback;
    return *this / sqrtf(L2);
}

vec4 pow(const vec4& a, const vec4& b) {
    return {powf(a.x, b.x), powf(a.y, b.y), powf(a.z, b.z), powf(a.w, b.w)};
}

vec4 round_up(const vec4& vec, float spacing) {
    return {round_up(vec.x, spacing), round_up(vec.y, spacing),
            round_up(vec.z, spacing), round_up(vec.w, spacing)};
}

vec4 round_down(const vec4& vec, float spacing) {
    return {round_down(vec.x, spacing), round_down(vec.y, spacing),
            round_down(vec.z, spacing), round_down(vec.w, spacing)};
}

vec4 round_nearest(const vec4& vec, float spacing) {
    return {round_nearest(vec.x, spacing), round_nearest(vec.y, spacing),
            round_nearest(vec.z, spacing), round_nearest(vec.w, spacing)};
}

bool is_rounded(const vec4& vec, float spacing) {
    return round_nearest(vec, spacing) == vec;
}

//   ▄▄▄▄         ▄▄▄
//  ██  ▀▀  ▄▄▄▄   ██   ▄▄▄▄  ▄▄▄▄▄
//  ██     ██  ██  ██  ██  ██ ██  ▀▀
//  ▀█▄▄█▀ ▀█▄▄█▀ ▄██▄ ▀█▄▄█▀ ██
//

void convert_from_hex(float* values, size_t num_values, const char* hex) {
    for (size_t i = 0; i < num_values; i++) {
        int c = 0;
        for (int j = 0; j < 2; j++) {
            c <<= 4;
            if (*hex >= '0' && *hex <= '9') {
                c += *hex - '0';
            } else if (*hex >= 'a' && *hex <= 'f') {
                c += *hex - 'a' + 10;
            } else if (*hex >= 'A' && *hex <= 'F') {
                c += *hex - 'A' + 10;
            } else {
                PLY_ASSERT(0);
            }
            hex++;
        }
        values[i] = c * (1 / 255.f);
    }
    PLY_ASSERT(*hex == 0);
}

vec3 srgb_to_linear(const vec3& vec) {
    return {srgb_to_linear(vec.x), srgb_to_linear(vec.y), srgb_to_linear(vec.z)};
}

vec4 srgb_to_linear(const vec4& vec) {
    return {srgb_to_linear(vec.x), srgb_to_linear(vec.y), srgb_to_linear(vec.z), vec.w};
}

vec3 linear_to_srgb(const vec3& vec) {
    return {linear_to_srgb(vec.x), linear_to_srgb(vec.y), linear_to_srgb(vec.z)};
}

vec4 linear_to_srgb(const vec4& vec) {
    return {linear_to_srgb(vec.x), linear_to_srgb(vec.y), linear_to_srgb(vec.z), vec.w};
}

//                   ▄▄    ▄▄▄▄          ▄▄▄▄
//  ▄▄▄▄▄▄▄   ▄▄▄▄  ▄██▄▄ ▀▀  ██ ▄▄  ▄▄ ▀▀  ██
//  ██ ██ ██  ▄▄▄██  ██    ▄█▀▀   ▀██▀   ▄█▀▀
//  ██ ██ ██ ▀█▄▄██  ▀█▄▄ ██▄▄▄▄ ▄█▀▀█▄ ██▄▄▄▄
//

mat2x2 mat2x2::identity() {
    return {{1, 0}, {0, 1}};
}

mat2x2 mat2x2::make_scale(const vec2& scale) {
    return {{scale.x, 0}, {0, scale.y}};
}

mat2x2 mat2x2::make_rotation(float radians) {
    return from_complex(Complex::from_angle(radians));
}

mat2x2 mat2x2::from_complex(const vec2& c) {
    return {{c.x, c.y}, {-c.y, c.x}};
}

mat2x2 mat2x2::transposed() const {
    PLY_PUN_SCOPE
    auto* m = reinterpret_cast<const float(*)[2]>(this);
    return {
        {m[0][0], m[1][0]},
        {m[0][1], m[1][1]},
    };
}

bool operator==(const mat2x2& a, const mat2x2& b) {
    return (a.col[0] == b.col[0]) && (a.col[1] == b.col[1]);
}

vec2 operator*(const mat2x2& m_, const vec2& v_) {
    vec2 result;
    {
        PLY_PUN_SCOPE
        auto* res = reinterpret_cast<float*>(&result);
        auto* m = reinterpret_cast<const float(*)[2]>(&m_);
        auto* v = reinterpret_cast<const float*>(&v_);
        for (ureg r = 0; r < 2; r++) {
            res[r] = m[0][r] * v[0] + m[1][r] * v[1];
        }
    }
    return result;
}

mat2x2 operator*(const mat2x2& a, const mat2x2& b) {
    mat2x2 result;
    for (ureg c = 0; c < 2; c++) {
        result[c] = a * b.col[c];
    }
    return result;
}

//                   ▄▄    ▄▄▄▄          ▄▄▄▄
//  ▄▄▄▄▄▄▄   ▄▄▄▄  ▄██▄▄ ▀▀  ██ ▄▄  ▄▄ ▀▀  ██
//  ██ ██ ██  ▄▄▄██  ██     ▀▀█▄  ▀██▀    ▀▀█▄
//  ██ ██ ██ ▀█▄▄██  ▀█▄▄ ▀█▄▄█▀ ▄█▀▀█▄ ▀█▄▄█▀
//

mat3x3::mat3x3(const mat3x4& m) : mat3x3{m.col[0], m.col[1], m.col[2]} {
}

mat3x3::mat3x3(const mat4x4& m)
    : mat3x3{vec3{m.col[0]}, vec3{m.col[1]}, vec3{m.col[2]}} {
}

mat3x3 mat3x3::identity() {
    return {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
}

mat3x3 mat3x3::make_scale(const vec3& arg) {
    return {{arg.x, 0, 0}, {0, arg.y, 0}, {0, 0, arg.z}};
}

mat3x3 mat3x3::make_rotation(const vec3& unit_axis, float radians) {
    return mat3x3::from_quaternion(Quaternion::from_axis_angle(unit_axis, radians));
}

mat3x3 mat3x3::from_quaternion(const Quaternion& q) {
    return {{1 - 2 * q.y * q.y - 2 * q.z * q.z, 2 * q.x * q.y + 2 * q.z * q.w,
             2 * q.x * q.z - 2 * q.y * q.w},
            {2 * q.x * q.y - 2 * q.z * q.w, 1 - 2 * q.x * q.x - 2 * q.z * q.z,
             2 * q.y * q.z + 2 * q.x * q.w},
            {2 * q.x * q.z + 2 * q.y * q.w, 2 * q.y * q.z - 2 * q.x * q.w,
             1 - 2 * q.x * q.x - 2 * q.y * q.y}};
}

bool mat3x3::has_scale() const {
    return !col[0].is_unit() || !col[1].is_unit() || !col[2].is_unit();
}

mat3x3 mat3x3::transposed() const {
    PLY_PUN_SCOPE
    auto* m = reinterpret_cast<const float(*)[3]>(this);
    return {
        {m[0][0], m[1][0], m[2][0]},
        {m[0][1], m[1][1], m[2][1]},
        {m[0][2], m[1][2], m[2][2]},
    };
}

bool operator==(const mat3x3& a_, const mat3x3& b_) {
    PLY_PUN_SCOPE
    auto* a = reinterpret_cast<const float*>(&a_);
    auto* b = reinterpret_cast<const float*>(&b_);
    for (ureg r = 0; r < 9; r++) {
        if (a[r] != b[r])
            return false;
    }
    return true;
}

vec3 operator*(const mat3x3& m_, const vec3& v_) {
    vec3 result;
    {
        PLY_PUN_SCOPE
        auto* res = reinterpret_cast<float*>(&result);
        auto* m = reinterpret_cast<const float(*)[3]>(&m_);
        auto* v = reinterpret_cast<const float*>(&v_);
        for (ureg r = 0; r < 3; r++) {
            res[r] = m[0][r] * v[0] + m[1][r] * v[1] + m[2][r] * v[2];
        }
    }
    return result;
}

mat3x3 operator*(const mat3x3& a, const mat3x3& b) {
    mat3x3 result;
    for (ureg c = 0; c < 3; c++) {
        result.col[c] = a * b.col[c];
    }
    return result;
}

//                   ▄▄    ▄▄▄▄            ▄▄▄
//  ▄▄▄▄▄▄▄   ▄▄▄▄  ▄██▄▄ ▀▀  ██ ▄▄  ▄▄  ▄█▀██
//  ██ ██ ██  ▄▄▄██  ██     ▀▀█▄  ▀██▀  ██▄▄██▄
//  ██ ██ ██ ▀█▄▄██  ▀█▄▄ ▀█▄▄█▀ ▄█▀▀█▄     ██
//

mat3x4::mat3x4(const mat3x3& m, const vec3& pos) {
    for (u32 i = 0; i < 3; i++) {
        col[i] = m.col[i];
    }
    col[3] = pos;
}

mat3x4::mat3x4(const mat4x4& m)
    : mat3x4{vec3{m.col[0]}, vec3{m.col[1]}, vec3{m.col[2]}, vec3{m.col[3]}} {
}

mat3x4 mat3x4::identity() {
    return {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {0, 0, 0}};
}

mat3x4 mat3x4::make_scale(const vec3& arg) {
    return {{arg.x, 0, 0}, {0, arg.y, 0}, {0, 0, arg.z}, {0, 0, 0}};
}

mat3x4 mat3x4::make_rotation(const vec3& unit_axis, float radians) {
    return mat3x4::from_quaternion(Quaternion::from_axis_angle(unit_axis, radians));
}

mat3x4 mat3x4::make_translation(const vec3& pos) {
    return {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}, pos};
}

mat3x4 mat3x4::from_quaternion(const Quaternion& q, const vec3& pos) {
    return {{1 - 2 * q.y * q.y - 2 * q.z * q.z, 2 * q.x * q.y + 2 * q.z * q.w,
             2 * q.x * q.z - 2 * q.y * q.w},
            {2 * q.x * q.y - 2 * q.z * q.w, 1 - 2 * q.x * q.x - 2 * q.z * q.z,
             2 * q.y * q.z + 2 * q.x * q.w},
            {2 * q.x * q.z + 2 * q.y * q.w, 2 * q.y * q.z - 2 * q.x * q.w,
             1 - 2 * q.x * q.x - 2 * q.y * q.y},
            pos};
}

mat3x4 mat3x4::inverted_ortho() const {
    mat3x4 result;
    reinterpret_cast<mat3x3&>(result) =
        reinterpret_cast<const mat3x3&>(*this).transposed();
    result.col[3] = reinterpret_cast<mat3x3&>(result) * -col[3];
    return result;
}

bool operator==(const mat3x4& a_, const mat3x4& b_) {
    PLY_PUN_SCOPE
    auto* a = reinterpret_cast<const float*>(&a_);
    auto* b = reinterpret_cast<const float*>(&b_);
    for (ureg r = 0; r < 12; r++) {
        if (a[r] != b[r])
            return false;
    }
    return true;
}

vec3 operator*(const mat3x4& m_, const vec3& v_) {
    vec3 result;
    {
        PLY_PUN_SCOPE
        auto* res = reinterpret_cast<float*>(&result);
        auto* m = reinterpret_cast<const float(*)[3]>(&m_);
        auto* v = reinterpret_cast<const float*>(&v_);
        for (ureg r = 0; r < 3; r++) {
            res[r] = m[0][r] * v[0] + m[1][r] * v[1] + m[2][r] * v[2] + m[3][r];
        }
    }
    return result;
}

vec4 operator*(const mat3x4& m_, const vec4& v_) {
    vec4 result;
    {
        PLY_PUN_SCOPE
        auto* res = reinterpret_cast<float*>(&result);
        auto* m = reinterpret_cast<const float(*)[3]>(&m_);
        auto* v = reinterpret_cast<const float*>(&v_);
        for (ureg r = 0; r < 3; r++) {
            res[r] = m[0][r] * v[0] + m[1][r] * v[1] + m[2][r] * v[2] + m[3][r] * v[3];
        }
        res[3] = v[3];
    }
    return result;
}

mat3x4 operator*(const mat3x4& a, const mat3x4& b) {
    mat3x4 result;
    for (ureg c = 0; c < 3; c++) {
        result.col[c] = a.as_mat3x3() * b.col[c];
    }
    result.col[3] = a * b.col[3];
    return result;
}

//                   ▄▄      ▄▄▄            ▄▄▄
//  ▄▄▄▄▄▄▄   ▄▄▄▄  ▄██▄▄  ▄█▀██  ▄▄  ▄▄  ▄█▀██
//  ██ ██ ██  ▄▄▄██  ██   ██▄▄██▄  ▀██▀  ██▄▄██▄
//  ██ ██ ██ ▀█▄▄██  ▀█▄▄     ██  ▄█▀▀█▄     ██
//

mat4x4::mat4x4(const mat3x3& m, const vec3& pos) {
    *this = {
        {m.col[0], 0},
        {m.col[1], 0},
        {m.col[2], 0},
        {pos, 1},
    };
}

mat4x4::mat4x4(const mat3x4& m) {
    *this = {
        {m.col[0], 0},
        {m.col[1], 0},
        {m.col[2], 0},
        {m.col[3], 1},
    };
}

mat4x4 mat4x4::identity() {
    return {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}};
}

mat4x4 mat4x4::make_scale(const vec3& arg) {
    return {{arg.x, 0, 0, 0}, {0, arg.y, 0, 0}, {0, 0, arg.z, 0}, {0, 0, 0, 1}};
}

mat4x4 mat4x4::make_rotation(const vec3& unit_axis, float radians) {
    return mat4x4::from_quaternion(Quaternion::from_axis_angle(unit_axis, radians));
}

mat4x4 mat4x4::make_translation(const vec3& pos) {
    return {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {pos, 1}};
}

mat4x4 mat4x4::from_quaternion(const Quaternion& q, const vec3& pos) {
    return {{1 - 2 * q.y * q.y - 2 * q.z * q.z, 2 * q.x * q.y + 2 * q.z * q.w,
             2 * q.x * q.z - 2 * q.y * q.w, 0},
            {2 * q.x * q.y - 2 * q.z * q.w, 1 - 2 * q.x * q.x - 2 * q.z * q.z,
             2 * q.y * q.z + 2 * q.x * q.w, 0},
            {2 * q.x * q.z + 2 * q.y * q.w, 2 * q.y * q.z - 2 * q.x * q.w,
             1 - 2 * q.x * q.x - 2 * q.y * q.y, 0},
            {pos, 1}};
}

mat4x4 mat4x4::make_projection(const Rect& frustum, float z_near, float z_far) {
    PLY_ASSERT(z_near > 0 && z_far > 0);
    mat4x4 result{0, 0, 0, 0};
    float oo_xdenom = 1.f / (frustum.maxs.x - frustum.mins.x);
    float oo_ydenom = 1.f / (frustum.maxs.y - frustum.mins.y);
    float oo_zdenom = 1.f / (z_near - z_far);
    result.col[0].x = 2.f * oo_xdenom;
    result.col[2].x = (frustum.mins.x + frustum.maxs.x) * oo_xdenom;
    result.col[1].y = 2.f * oo_ydenom;
    result.col[2].y = (frustum.mins.y + frustum.maxs.y) * oo_xdenom;
    result.col[2].z = (z_near + z_far) * oo_zdenom;
    result.col[2].w = -1.f;
    result.col[3].z = (2 * z_near * z_far) * oo_zdenom;
    return result;
}

mat4x4 mat4x4::make_ortho(const Rect& rect, float z_near, float z_far) {
    mat4x4 result{0, 0, 0, 0};
    float tow = 2 / rect.width();
    float toh = 2 / rect.height();
    float oo_zrange = 1 / (z_near - z_far);
    result.col[0].x = tow;
    result.col[3].x = -rect.mid().x * tow;
    result.col[1].y = toh;
    result.col[3].y = -rect.mid().y * toh;
    result.col[2].z = 2 * oo_zrange;
    result.col[3].z = (z_near + z_far) * oo_zrange;
    result.col[3].w = 1.f;
    return result;
}

mat4x4 mat4x4::transposed() const {
    PLY_PUN_SCOPE
    auto* m = reinterpret_cast<const float(*)[4]>(this);
    return {
        {m[0][0], m[1][0], m[2][0], m[3][0]},
        {m[0][1], m[1][1], m[2][1], m[3][1]},
        {m[0][2], m[1][2], m[2][2], m[3][2]},
        {m[0][3], m[1][3], m[2][3], m[3][3]},
    };
}

mat4x4 mat4x4::inverted_ortho() const {
    mat4x4 result = transposed();
    result.col[0].w = 0;
    result.col[1].w = 0;
    result.col[2].w = 0;
    result.col[3] = result * -col[3];
    result.col[3].w = 1;
    return result;
}

bool operator==(const mat4x4& a_, const mat4x4& b_) {
    PLY_PUN_SCOPE
    auto* a = reinterpret_cast<const float*>(&a_);
    auto* b = reinterpret_cast<const float*>(&b_);
    for (ureg r = 0; r < 16; r++) {
        if (a[r] != b[r])
            return false;
    }
    return true;
}

vec4 operator*(const mat4x4& m_, const vec4& v_) {
    vec4 result;
    {
        PLY_PUN_SCOPE
        auto* res = reinterpret_cast<float*>(&result);
        auto* m = reinterpret_cast<const float(*)[4]>(&m_);
        auto* v = reinterpret_cast<const float*>(&v_);
        for (ureg r = 0; r < 4; r++) {
            res[r] = m[0][r] * v[0] + m[1][r] * v[1] + m[2][r] * v[2] + m[3][r] * v[3];
        }
    }
    return result;
}

mat4x4 operator*(const mat4x4& a, const mat4x4& b) {
    mat4x4 result;
    for (ureg c = 0; c < 4; c++) {
        result.col[c] = a * b.col[c];
    }
    return result;
}

mat4x4 operator*(const mat3x4& a, const mat4x4& b) {
    mat4x4 result;
    for (ureg c = 0; c < 4; c++) {
        result[c] = a * b.col[c];
    }
    return result;
}

mat4x4 operator*(const mat4x4& a, const mat3x4& b) {
    mat4x4 result;
    for (ureg c = 0; c < 3; c++) {
        result.col[c] = a * vec4{b.col[c], 0};
    }
    result[3] = a * vec4{b.col[3], 1};
    return result;
}

//   ▄▄▄▄         ▄▄        ▄▄▄▄▄          ▄▄
//  ██  ██ ▄▄  ▄▄ ▄▄  ▄▄▄▄  ██  ██  ▄▄▄▄  ▄██▄▄
//  ██▀▀██  ▀██▀  ██ ▀█▄▄▄  ██▀▀█▄ ██  ██  ██
//  ██  ██ ▄█▀▀█▄ ██  ▄▄▄█▀ ██  ██ ▀█▄▄█▀  ▀█▄▄
//

AxisRot AxisRot::make_basis(Axis v, u32 i) {
    PLY_ASSERT(i < 3);
    AxisRot r;
    r.cols[i++] = v;
    i -= (i >= 3) * 3;
    u32 v2 = u32(v) + 2;
    v2 -= (v2 >= 6) * 6;
    r.cols[i++] = Axis(v2);
    i -= (i >= 3) * 3;
    r.cols[i] = cross(v, Axis(v2));
    return r;
}

bool AxisRot::is_valid() const {
    return ply::is_valid(cols[0]) && ply::is_valid(cols[1]) &&
            ply::is_valid(cols[2]);
}

bool AxisRot::is_ortho() const {
    return is_perp(cols[0], cols[1]) && is_perp(cols[1], cols[2]) &&
            is_perp(cols[2], cols[0]);
}

bool AxisRot::is_right_handed() const {
    PLY_ASSERT(is_ortho());
    return dot(cross(cols[0], cols[1]), cols[2]) > 0;
}

AxisRot AxisRot::inverted() const {
    PLY_ASSERT(is_ortho());
    AxisRot r = identity();
    for (u32 i = 0; i < 3; i++) {
        u32 img = u32(cols[i]);
        r.cols[img >> 1] = Axis((i << 1) ^ (img & 1));
    }
    return r;
}

Axis operator*(const AxisRot& a, Axis b) {
    return Axis(u32(a.cols[u32(b) >> 1]) ^ (u32(b) & 1));
}

vec3 operator*(const AxisRot& a, const vec3& v) {
    vec3 r{0, 0, 0};
    {
        PLY_PUN_SCOPE
        auto* r_ = reinterpret_cast<float*>(&r);
        r_[u32(a.cols[0]) >> 1] = v.x * sgn(a.cols[0]);
        r_[u32(a.cols[1]) >> 1] = v.y * sgn(a.cols[1]);
        r_[u32(a.cols[2]) >> 1] = v.z * sgn(a.cols[2]);
    }
    return r;
}

AxisRot operator*(const AxisRot& a, const AxisRot& b) {
    return {a * b.cols[0], a * b.cols[1], a * b.cols[2]};
}

bool operator==(const AxisRot& a, const AxisRot& b) {
    return a.cols[0] == b.cols[0] && a.cols[1] == b.cols[1] && a.cols[2] == b.cols[2];
}

//   ▄▄▄▄                 ▄▄                        ▄▄
//  ██  ██ ▄▄  ▄▄  ▄▄▄▄  ▄██▄▄  ▄▄▄▄  ▄▄▄▄▄  ▄▄▄▄▄  ▄▄  ▄▄▄▄  ▄▄▄▄▄
//  ██  ██ ██  ██  ▄▄▄██  ██   ██▄▄██ ██  ▀▀ ██  ██ ██ ██  ██ ██  ██
//  ▀█▄▄█▀ ▀█▄▄██ ▀█▄▄██  ▀█▄▄ ▀█▄▄▄  ██     ██  ██ ██ ▀█▄▄█▀ ██  ██
//      ▀▀

Quaternion Quaternion::from_axis_angle(const vec3& unit_axis, float radians) {
    PLY_ASSERT(unit_axis.is_unit());
    float c = cosf(radians / 2);
    float s = sinf(radians / 2);
    return {s * unit_axis.x, s * unit_axis.y, s * unit_axis.z, c};
}

Quaternion Quaternion::from_unit_vectors(const vec3& start, const vec3& end) {
    // vec4{cross(start, end), dot(start, end)} gives you double the desired rotation.
    // To get the desired rotation, "average" (really just sum) that with vec4{0, 0,
    // 0, 1}, then normalize.
    float w = dot(start, end) + 1;
    if (w < 1e-6f) {
        // Exceptional case: Vectors point in opposite directions.
        // Choose a perpendicular axis and make a 180 degree rotation.
        vec3 not_collinear = (fabsf(start.x) < 0.9f) ? vec3{1, 0, 0} : vec3{0, 1, 0};
        vec3 axis = cross(start, not_collinear);
        return vec4{axis, 0}.normalized().as_quaternion();
    }
    vec3 v = cross(start, end);
    return vec4{v, w}.normalized().as_quaternion();
}

template <typename M>
Quaternion quaternion_from_ortho(M m) {
    float t; // This will be set to 4*c*c for some quaternion component c.
    // At least one component's square must be >= 1/4. (Otherwise, it isn't a unit
    // quaternion.) Let's require t >= 1/2. This will accept any component whose square
    // is >= 1/8.
    if ((t = 1.f + m[0][0] + m[1][1] + m[2][2]) >= 0.5f) { // 4*w*w
        float w = sqrtf(t) * 0.5f;
        float f = 0.25f / w;
        return {(m[1][2] - m[2][1]) * f, (m[2][0] - m[0][2]) * f,
                (m[0][1] - m[1][0]) * f, w};
    } else if ((t = 1.f + m[0][0] - m[1][1] - m[2][2]) >= 0.5f) { // 4*x*x
        // Prefer positive w component in result
        float wco = m[1][2] - m[2][1];
        float x =
            sqrtf(t) * ((wco >= 0) - 0.5f); // equivalent to sqrtf(t) * 0.5f * sgn(wco)
        float f = 0.25f / x;
        return {x, (m[0][1] + m[1][0]) * f, (m[2][0] + m[0][2]) * f, wco * f};
    } else if ((t = 1.f - m[0][0] + m[1][1] - m[2][2]) >= 0.5f) { // 4*y*y
        float wco = m[2][0] - m[0][2];
        float y =
            sqrtf(t) * ((wco >= 0) - 0.5f); // equivalent to sqrtf(t) * 0.5f * sgn(wco)
        float f = 0.25f / y;
        return {(m[0][1] + m[1][0]) * f, y, (m[1][2] + m[2][1]) * f, wco * f};
    } else if ((t = 1.f - m[0][0] - m[1][1] + m[2][2]) >= 0.5f) { // 4*z*z
        float wco = m[0][1] - m[1][0];
        float z =
            sqrtf(t) * ((wco >= 0) - 0.5f); // equivalent to sqrtf(t) * 0.5f * sgn(wco)
        float f = 0.25f / z;
        return {(m[2][0] + m[0][2]) * f, (m[1][2] + m[2][1]) * f, z, wco * f};
    }
    PLY_ASSERT(0); // The matrix is not even close to being orthonormal
    return {0, 0, 0, 1};
}

Quaternion Quaternion::from_ortho(const mat3x3& m) {
    PLY_PUN_SCOPE
    return quaternion_from_ortho(reinterpret_cast<const float(*)[3]>(&m));
}

Quaternion Quaternion::from_ortho(const mat4x4& m) {
    PLY_PUN_SCOPE
    return quaternion_from_ortho(reinterpret_cast<const float(*)[4]>(&m));
}

vec3 Quaternion::rotate_unit_x() const {
    return {1 - 2 * y * y - 2 * z * z, 2 * x * y + 2 * z * w, 2 * x * z - 2 * y * w};
}

vec3 Quaternion::rotate_unit_y() const {
    return {2 * x * y - 2 * z * w, 1 - 2 * x * x - 2 * z * z, 2 * y * z + 2 * x * w};
}

vec3 Quaternion::rotate_unit_z() const {
    return {2 * x * z + 2 * y * w, 2 * y * z - 2 * x * w, 1 - 2 * x * x - 2 * y * y};
}

Quaternion Quaternion::negated_if_closer_to(const Quaternion& other) const {
    const vec4& v0 = this->as_vec4();
    const vec4& v1 = other.as_vec4();
    return (square(v0 - v1) < square(-v0 - v1) ? v0 : -v0).as_quaternion();
}

vec3 operator*(const Quaternion& q, const vec3& v) {
    // From https://gist.github.com/rygorous/8da6651b597f3d825862
    vec3 t = cross((const vec3&) q, v) * 2.f;
    return v + t * q.w + cross((const vec3&) q, t);
}

Quaternion operator*(const Quaternion& a, const Quaternion& b) {
    return {a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
            a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
            a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
            a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z};
}

Quaternion mix(const Quaternion& a, const Quaternion& b, float t) {
    vec4 linear_mix = mix((const vec4&) a.negated_if_closer_to(b), (const vec4&) b, t);
    return linear_mix.normalized().as_quaternion();
}

//   ▄▄▄▄                 ▄▄   ▄▄▄▄▄
//  ██  ██ ▄▄  ▄▄  ▄▄▄▄  ▄██▄▄ ██  ██  ▄▄▄▄   ▄▄▄▄
//  ██  ██ ██  ██  ▄▄▄██  ██   ██▀▀▀  ██  ██ ▀█▄▄▄
//  ▀█▄▄█▀ ▀█▄▄██ ▀█▄▄██  ▀█▄▄ ██     ▀█▄▄█▀  ▄▄▄█▀
//      ▀▀

QuatPos QuatPos::inverted() const {
    Quaternion qi = quat.inverted();
    return {qi, qi * -pos};
}

QuatPos QuatPos::identity() {
    return {{0, 0, 0, 1}, {0, 0, 0}};
}

QuatPos QuatPos::make_translation(const vec3& pos) {
    return {{0, 0, 0, 1}, pos};
}

QuatPos QuatPos::make_rotation(const vec3& unit_axis, float radians) {
    return {Quaternion::from_axis_angle(unit_axis, radians), {0, 0, 0}};
}

QuatPos QuatPos::from_ortho(const mat3x4& m) {
    return {Quaternion::from_ortho(m.as_mat3x3()), m[3]};
}

QuatPos QuatPos::from_ortho(const mat4x4& m) {
    return {Quaternion::from_ortho(m), vec3{m[3]}};
}

} // namespace ply
