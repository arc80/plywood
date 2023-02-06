/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-math/Core.h>
#include <ply-math/Vector.h>
#include <ply-math/Matrix.h>

namespace ply {

//----------------------------------------------
// Axis3
//----------------------------------------------
enum class Axis3 : u8 { XPos = 0, XNeg, YPos, YNeg, ZPos, ZNeg, Count };

inline bool is_valid(Axis3 vec) {
    return vec < Axis3::Count;
}

inline Axis3 abs(Axis3 vec) {
    return Axis3(s32(vec) & 6);
}

inline s32 sgn(Axis3 vec) {
    return 1 - (s32(vec) & 1) * 2;
}

inline bool is_perp(Axis3 va, Axis3 vb) {
    return ((s32(va) ^ s32(vb)) & 6) != 0;
}

inline Axis3 cross(Axis3 va, Axis3 vb) {
    s32 a = s32(va);
    s32 b = s32(vb);
    s32 diff = b - (a & 6);
    PLY_ASSERT((diff & ~1) != 0); // must be perpendicular
    diff += (diff < 0) * 6;
    return Axis3(6 ^ a ^ b ^ (diff >> 2));
}

inline s32 dot(Axis3 va, Axis3 vb) {
    s32 x = s32(va) ^ s32(vb);
    s32 mask = ((x & 6) != 0) - 1;
    s32 sgn = 1 - (x & 1) * 2;
    return sgn & mask;
}

PLY_INLINE float dot(Axis3 va, const Float3& vb) {
    PLY_PUN_SCOPE
    auto* vb_ = reinterpret_cast<const float*>(&vb);
    s32 a = s32(va);
    s32 sgn = 1 - (a & 1) * 2;
    return vb_[a >> 1] * sgn;
}

inline Axis3 negate(Axis3 axis3_d) {
    return (Axis3) (u8(axis3_d) ^ 1u);
}

inline Axis3 mul_sign(Axis3 axis3_d, s32 sgn) {
    PLY_ASSERT(sgn == 1 || sgn == -1);
    return Axis3(u32(axis3_d) ^ (sgn < 0));
}

inline const Float3& to_float3(Axis3 axis3_d) {
    static Float3 table[] = {{1, 0, 0},  {-1, 0, 0}, {0, 1, 0},
                             {0, -1, 0}, {0, 0, 1},  {0, 0, -1}};
    PLY_ASSERT((u32) axis3_d < PLY_STATIC_ARRAY_SIZE(table));
    return table[(u32) axis3_d];
}

//----------------------------------------------
// Axis2
//----------------------------------------------
enum class Axis2 : u8 { XPos = 0, YPos, XNeg, YNeg, Count };

inline bool is_valid(Axis2 vec) {
    return vec < Axis2::Count;
}

inline s32 sgn(Axis2 a) {
    return 1 - (u32(a) >> 1) * 2;
}

inline Axis2 negate(Axis2 vec) {
    return Axis2((4u - u32(vec)) & 3u);
}

inline Float2 operator*(Axis2 rot, const Float2& v) {
    Float2 r;
    {
        PLY_PUN_SCOPE
        auto* r_ = reinterpret_cast<float*>(&r);
        r_[u32(rot) & 1] = v.x * sgn(rot);
        r_[1 - (u32(rot) & 1)] = v.y * sgn(Axis2((u32(rot) + 1) & 3));
    }
    return r;
}

inline Float3 operator*(Axis2 rot, const Float3& v) {
    return {rot * v.as_float2(), v.z};
}

inline const Float3& to_float3(Axis2 axis2_d) {
    static const Float3 table[] = {{1, 0, 0}, {0, 1, 0}, {-1, 0, 0}, {0, -1, 0}};
    PLY_ASSERT((u32) axis2_d < PLY_STATIC_ARRAY_SIZE(table));
    return table[(u32) axis2_d];
}

inline const Float2& to_float2(Axis2 axis2_d) {
    return to_float3(axis2_d).as_float2();
}

inline Axis3 to_axis3(Axis2 axis2_d) {
    static const Axis3 table[] = {Axis3::XPos, Axis3::YPos, Axis3::XNeg, Axis3::YNeg};
    PLY_ASSERT((u32) axis2_d < PLY_STATIC_ARRAY_SIZE(table));
    return table[(u32) axis2_d];
}

inline Axis2 to_axis2(Axis3 axis3_d) {
    static const Axis2 table[] = {Axis2::XPos, Axis2::XNeg, Axis2::YPos, Axis2::YNeg};
    PLY_ASSERT((u32) axis3_d < PLY_STATIC_ARRAY_SIZE(table));
    return table[(u32) axis3_d];
}

//----------------------------------------------
// AxisRot
//----------------------------------------------
struct AxisRot {
    Axis3 cols[3];

    AxisRot() = default;

    AxisRot(Axis3 x_img, Axis3 y_img, Axis3 z_img) {
        cols[0] = x_img;
        cols[1] = y_img;
        cols[2] = z_img;
    }

    AxisRot(Axis3 x_img, Axis3 y_img) : AxisRot{x_img, y_img, cross(x_img, y_img)} {
    }

    static AxisRot identity() {
        return {Axis3::XPos, Axis3::YPos, Axis3::ZPos};
    }

    static AxisRot make_basis(Axis3 v, u32 i) {
        PLY_ASSERT(i < 3);
        AxisRot r;
        r.cols[i++] = v;
        i -= (i >= 3) * 3;
        u32 v2 = u32(v) + 2;
        v2 -= (v2 >= 6) * 6;
        r.cols[i++] = Axis3(v2);
        i -= (i >= 3) * 3;
        r.cols[i] = cross(v, Axis3(v2));
        return r;
    }

    static AxisRot from_rot2_d(Axis2 rot) {
        return {to_axis3(rot), to_axis3(Axis2((u32(rot) + 1) & 3)), Axis3::ZPos};
    }

    bool is_valid() const {
        return ply::is_valid(cols[0]) && ply::is_valid(cols[1]) &&
               ply::is_valid(cols[2]);
    }

    bool is_ortho() const {
        return is_perp(cols[0], cols[1]) && is_perp(cols[1], cols[2]) &&
               is_perp(cols[2], cols[0]);
    }

    bool is_right_handed() const {
        PLY_ASSERT(is_ortho());
        return dot(cross(cols[0], cols[1]), cols[2]) > 0;
    }

    Axis3 operator[](u32 i) const {
        PLY_ASSERT(i < 3);
        return cols[i];
    }

    Axis3 operator*(Axis3 v) const {
        u32 a = u32(v);
        return Axis3(u32(cols[a >> 1]) ^ (a & 1));
    }

    Axis3 operator*(Axis2 v) const {
        return *this * to_axis3(v);
    }

    Float3 operator*(const Float3& v) const {
        PLY_ASSERT(is_ortho());
        Float3 r = {0, 0, 0};
        {
            PLY_PUN_SCOPE
            auto* r_ = reinterpret_cast<float*>(&r);
            r_[u32(cols[0]) >> 1] = v.x * sgn(cols[0]);
            r_[u32(cols[1]) >> 1] = v.y * sgn(cols[1]);
            r_[u32(cols[2]) >> 1] = v.z * sgn(cols[2]);
        }
        return r;
    }

    AxisRot operator*(AxisRot other) const {
        return {*this * other[0], *this * other[1], *this * other[2]};
    }

    bool operator==(AxisRot other) const {
        return cols[0] == other.cols[0] && cols[1] == other.cols[1] &&
               cols[2] == other.cols[2];
    }

    bool operator!=(AxisRot other) const {
        return !(*this == other);
    }

    bool operator<(AxisRot other) const {
        if (cols[0] != other.cols[0]) {
            return cols[0] < other.cols[0];
        } else if (cols[1] != other.cols[1]) {
            return cols[1] < other.cols[1];
        } else {
            return cols[2] < other.cols[2];
        }
    }

    AxisRot inverted() const {
        PLY_ASSERT(is_ortho());
        AxisRot r = identity();
        for (u32 i = 0; i < 3; i++) {
            u32 img = u32(cols[i]);
            r.cols[img >> 1] = Axis3((i << 1) ^ (img & 1));
        }
        return r;
    }

    Float3x3 to_float3x3() const {
        return {to_float3(cols[0]), to_float3(cols[1]), to_float3(cols[2])};
    }

    Float4x4 to_float4x4() const {
        return {{to_float3(cols[0]), 0},
                {to_float3(cols[1]), 0},
                {to_float3(cols[2]), 0},
                {0, 0, 0, 1}};
    }

    template <typename Callback>
    static void for_each(const Callback& cb) {
        for (u32 x_img = 0; x_img < 6; x_img++) {
            for (u32 y_img = 0; y_img < 6; y_img++) {
                if ((x_img ^ y_img) & 6) {
                    Axis3 z_img = cross(Axis3(x_img), Axis3(y_img));
                    cb(AxisRot{Axis3(x_img), Axis3(y_img), z_img});
                    cb(AxisRot{Axis3(x_img), Axis3(y_img), negate(z_img)});
                }
            }
        }
    };
};

struct AxisRotPos {
    AxisRot rot;
    Float3 pos;

    AxisRotPos() = default;

    AxisRotPos(AxisRot rot, const Float3& pos = {0, 0, 0}) : rot{rot}, pos{pos} {
    }

    AxisRotPos operator*(const AxisRotPos& arg) const {
        return {rot * arg.rot, rot * arg.pos + pos};
    }

    AxisRotPos operator*(AxisRot arg) const {
        return {rot * arg, pos};
    }

    friend AxisRotPos operator*(AxisRot arg, const AxisRotPos& rot_pos) {
        return {arg * rot_pos.rot, arg * rot_pos.pos};
    }

    Float3 operator*(const Float3& v) const {
        return rot * v + pos;
    }

    bool operator==(const AxisRotPos& other) const {
        return rot == other.rot && pos == other.pos;
    }

    AxisRotPos inverted() const {
        AxisRot roti = rot.inverted();
        return {roti, roti * -pos};
    }

    static AxisRotPos identity() {
        return {AxisRot::identity(), {0, 0, 0}};
    }

    static AxisRotPos make_translation(const Float3& pos) {
        return {AxisRot::identity(), pos};
    }

    Float4x4 to_float4x4() const {
        return Float4x4{rot.to_float3x3()};
    }
};

//-------------------------------------

// Separate this?
enum class Reflection : u8 {
    XPosYPos = 0,
    YPosXNeg,
    XNegYNeg,
    YNegXPos,
    XNegYPos,
    YPosXPos,
    XPosYNeg,
    YNegXNeg,
    Count
};

inline AxisRot to_axis_rot(Reflection refl) {
    u32 i = u32(refl);
    PLY_ASSERT(i < 4);
    return {to_axis3(Axis2(i)), to_axis3(Axis2((i + 1) & 3)), Axis3::ZPos};
}

extern const Float2x2 ReflectXform[(u32) Reflection::Count];

enum class Symmetry : u8 {
    Identity = 0,
    XMirror,
    YMirror,
    Rot180,
    XYMirror,
    Rot90,
    Rot90Mirror, // all
    Count
};

} // namespace ply
