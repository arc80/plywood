/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-math/Core.h>
#include <ply-math/Vector.h>
#include <ply-math/Matrix.h>

namespace ply {

//----------------------------------------------
// Axis3
//----------------------------------------------
enum class Axis3 : u8 { XPos = 0, XNeg, YPos, YNeg, ZPos, ZNeg, Count };

inline bool isValid(Axis3 vec) {
    return vec < Axis3::Count;
}

inline Axis3 abs(Axis3 vec) {
    return Axis3(s32(vec) & 6);
}

inline s32 sgn(Axis3 vec) {
    return 1 - (s32(vec) & 1) * 2;
}

inline bool isPerp(Axis3 va, Axis3 vb) {
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

inline Axis3 negate(Axis3 axis3D) {
    return (Axis3)(u8(axis3D) ^ 1u);
}

inline Axis3 mulSign(Axis3 axis3D, s32 sgn) {
    PLY_ASSERT(sgn == 1 || sgn == -1);
    return Axis3(u32(axis3D) ^ (sgn < 0));
}

inline const Float3& toFloat3(Axis3 axis3D) {
    static Float3 table[] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    PLY_ASSERT((u32) axis3D < PLY_STATIC_ARRAY_SIZE(table));
    return table[(u32) axis3D];
}

//----------------------------------------------
// Axis2
//----------------------------------------------
enum class Axis2 : u8 { XPos = 0, YPos, XNeg, YNeg, Count };

inline bool isValid(Axis2 vec) {
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
    return {rot * v.asFloat2(), v.z};
}

inline const Float3& toFloat3(Axis2 axis2D) {
    static const Float3 table[] = {{1, 0, 0}, {0, 1, 0}, {-1, 0, 0}, {0, -1, 0}};
    PLY_ASSERT((u32) axis2D < PLY_STATIC_ARRAY_SIZE(table));
    return table[(u32) axis2D];
}

inline const Float2& toFloat2(Axis2 axis2D) {
    return toFloat3(axis2D).asFloat2();
}

inline Axis3 toAxis3(Axis2 axis2D) {
    static const Axis3 table[] = {Axis3::XPos, Axis3::YPos, Axis3::XNeg, Axis3::YNeg};
    PLY_ASSERT((u32) axis2D < PLY_STATIC_ARRAY_SIZE(table));
    return table[(u32) axis2D];
}

inline Axis2 toAxis2(Axis3 axis3D) {
    static const Axis2 table[] = {Axis2::XPos, Axis2::XNeg, Axis2::YPos, Axis2::YNeg};
    PLY_ASSERT((u32) axis3D < PLY_STATIC_ARRAY_SIZE(table));
    return table[(u32) axis3D];
}

//----------------------------------------------
// AxisRot
//----------------------------------------------
struct AxisRot {
    Axis3 cols[3];

    AxisRot() = default;

    AxisRot(Axis3 xImg, Axis3 yImg, Axis3 zImg) {
        cols[0] = xImg;
        cols[1] = yImg;
        cols[2] = zImg;
    }

    AxisRot(Axis3 xImg, Axis3 yImg) : AxisRot{xImg, yImg, cross(xImg, yImg)} {
    }

    static AxisRot identity() {
        return {Axis3::XPos, Axis3::YPos, Axis3::ZPos};
    }

    static AxisRot makeBasis(Axis3 v, u32 i) {
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

    static AxisRot fromRot2D(Axis2 rot) {
        return {toAxis3(rot), toAxis3(Axis2((u32(rot) + 1) & 3)), Axis3::ZPos};
    }

    bool isValid() const {
        return ply::isValid(cols[0]) && ply::isValid(cols[1]) && ply::isValid(cols[2]);
    }

    bool isOrtho() const {
        return isPerp(cols[0], cols[1]) && isPerp(cols[1], cols[2]) && isPerp(cols[2], cols[0]);
    }

    bool isRightHanded() const {
        PLY_ASSERT(isOrtho());
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
        return *this * toAxis3(v);
    }

    Float3 operator*(const Float3& v) const {
        PLY_ASSERT(isOrtho());
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
        return cols[0] == other.cols[0] && cols[1] == other.cols[1] && cols[2] == other.cols[2];
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
        PLY_ASSERT(isOrtho());
        AxisRot r = identity();
        for (u32 i = 0; i < 3; i++) {
            u32 img = u32(cols[i]);
            r.cols[img >> 1] = Axis3((i << 1) ^ (img & 1));
        }
        return r;
    }

    Float3x3 toFloat3x3() const {
        return {toFloat3(cols[0]), toFloat3(cols[1]), toFloat3(cols[2])};
    }

    Float4x4 toFloat4x4() const {
        return {
            {toFloat3(cols[0]), 0}, {toFloat3(cols[1]), 0}, {toFloat3(cols[2]), 0}, {0, 0, 0, 1}};
    }

    template <typename Hasher>
    void appendTo(Hasher& hasher) const {
        PLY_ASSERT(isValid());
        u32 value = u32(cols[0]) | (u32(cols[1]) << 8) | (u32(cols[2]) << 16);
        hasher.append(value);
    }

    template <typename Callback>
    static void forEach(const Callback& cb) {
        for (u32 xImg = 0; xImg < 6; xImg++) {
            for (u32 yImg = 0; yImg < 6; yImg++) {
                if ((xImg ^ yImg) & 6) {
                    Axis3 zImg = cross(Axis3(xImg), Axis3(yImg));
                    cb(AxisRot{Axis3(xImg), Axis3(yImg), zImg});
                    cb(AxisRot{Axis3(xImg), Axis3(yImg), negate(zImg)});
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

    friend AxisRotPos operator*(AxisRot arg, const AxisRotPos& rotPos) {
        return {arg * rotPos.rot, arg * rotPos.pos};
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

    static AxisRotPos makeTranslation(const Float3& pos) {
        return {AxisRot::identity(), pos};
    }

    template <typename Hasher>
    void appendTo(Hasher& hasher) const {
        rot.appendTo(hasher);
        pos.appendTo(hasher);
    }

    Float4x4 toFloat4x4() const {
        return rot.toFloat3x3().toFloat4x4(pos);
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

inline AxisRot toAxisRot(Reflection refl) {
    u32 i = u32(refl);
    PLY_ASSERT(i < 4);
    return {toAxis3(Axis2(i)), toAxis3(Axis2((i + 1) & 3)), Axis3::ZPos};
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
