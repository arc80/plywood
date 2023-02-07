﻿/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-math/Core.h>
#include <ply-math/Matrix.h>

namespace ply {

//----------------------------------------------------
// ScalePos2
//----------------------------------------------------

struct ScalePos2 {
    Float2 scale;
    Float2 pos;

    ScalePos2() {
    }

    ScalePos2(const Float2& scale, const Float2& pos) : scale(scale), pos(pos) {
    }

    static ScalePos2 identity() {
        return {{1, 1}, {0, 0}};
    }

    static ScalePos2 make_scale(const Float2& scale) {
        return {scale, 0};
    }

    static ScalePos2 make_translation(const Float2& pos) {
        return {{0, 0}, pos};
    }

    static ScalePos2 map_to_rect(const Rect& r) {
        return {r.size(), r.mins};
    }

    ScalePos2 operator*(const ScalePos2& arg) const {
        return {scale * arg.scale, (scale * arg.pos) + pos};
    }

    Float2 operator*(const Float2& p) const {
        return (scale * p) + pos;
    }

    ScalePos2 inverted() const {
        Float2 oo_scale = 1.f / scale;
        return {oo_scale, -pos * oo_scale};
    }

    const Float4& as_float4() const {
        return reinterpret_cast<const Float4&>(*this);
    }
};

inline Rect operator*(const ScalePos2& xform, const Rect& r) {
    return {xform * r.mins, xform * r.maxs};
}

//----------------------------------------------------
// QuatPos
//----------------------------------------------------

//------------------------------------------------------------------------------------------------
/*!
A transformation consisting of a rotation followed by a translation.

A `QuatPos` behaves like a `Float4x4` or `Float3x4` that performs only rotation and
translation. While `Float4x4` uses 64 bytes of storage and `Float3x4` uses 48 bytes,
`QuatPos` uses only 28 bytes.
*/
struct QuatPos {
    /*!
    The rotation component.
    */
    Quaternion quat;
    /*!
    The translation component.
    */
    Float3 pos;

    /*!
    \category Constructors
    Constructs an uninitialized `QuatPos`.
    */
    QuatPos() = default;
    /*!
    Constructs a `QuatPos` from a quaternion.
    */
    explicit QuatPos(const Quaternion& quat) : quat(quat) {
    }
    /*!
    Constructs a `QuatPos` from a quaternion and a translation.
    */
    QuatPos(const Quaternion& quat, const Float3& pos) : quat(quat), pos(pos) {
    }
    /*!
    \category Transformation Functions
    Returns a `QuatPos` that performs the inverse transformation of the given `QuatPos`.
    */
    QuatPos inverted() const;
    /*!
    \category Creation Functions
    Returns the identity `QuatPos{{0, 0, 0, 1}, {0, 0, 0}}`.
    */
    static QuatPos identity();
    /*!
    Returns a `QuatPos` that performs translation only.
    */
    static QuatPos make_translation(const Float3& pos);
    /*!
    Returns a `QuatPos` that performs rotation only. `unit_axis` must have unit length.
    The angle is specified in radians. Rotation follows the [right-hand
    rule](https://en.wikipedia.org/wiki/Right-hand_rule#Rotations) in a right-handed
    coordinate system.
    */
    static QuatPos make_rotation(const Float3& unit_axis, float radians);
    /*!
    \begin_group
    Converts a transformation matrix to a `QuatPos`. The matrix must only consist of a
    rotation and/or translation component.
    */
    static QuatPos from_ortho(const Float3x4& m);
    static QuatPos from_ortho(const Float4x4& m);
    /*!
    \end_group
    */
};

/*!
\add_to_class QuatPos
\category Transformation Functions
Transforms `v` using `qp`. Equivalent to `qp.quat * v + qp.pos`.
*/
inline Float3 operator*(const QuatPos& qp, const Float3& v) {
    return (qp.quat * v) + qp.pos;
}
/*!
\begin_group
Composes a `QuatPos` with another `QuatPos` or a `Quaternion`. The resulting `QuatPos`
performs the transformation performed by `b` followed by the transformation performed by
`a`.
*/
inline QuatPos operator*(const QuatPos& a, const QuatPos& b) {
    return {a.quat * b.quat, (a.quat * b.pos) + a.pos};
}
inline QuatPos operator*(const QuatPos& a, const Quaternion& b) {
    return {a.quat * b, a.pos};
}
inline QuatPos operator*(const Quaternion& a, const QuatPos& b) {
    return {a * b.quat, a * b.pos};
}
/*!
\end_group
*/

inline Float3x4 Float3x4::from_quat_pos(const QuatPos& qp) {
    return from_quaternion(qp.quat, qp.pos);
}
inline Float4x4 Float4x4::from_quat_pos(const QuatPos& qp) {
    return from_quaternion(qp.quat, qp.pos);
}

//----------------------------------------------------
// QuatPosScale
//----------------------------------------------------

struct QuatPosScale {
    Quaternion quat;
    Float3 pos;
    Float3 scale;

    QuatPosScale() {
    }

    QuatPosScale(const QuatPos& qp, const Float3& scale = {1, 1, 1})
        : quat{qp.quat}, pos{qp.pos}, scale{scale} {
    }

    QuatPosScale(const Quaternion& quat, const Float3& pos, const Float3& scale)
        : quat{quat}, pos{pos}, scale{scale} {
    }

    static QuatPosScale identity() {
        return {Quaternion::identity(), {0, 0, 0}, {1, 1, 1}};
    }

    static QuatPosScale make_translation(const Float3& pos) {
        return {{0, 0, 0, 1}, pos, {1, 1, 1}};
    }

    static QuatPosScale make_scale(const Float3& scale) {
        return {{0, 0, 0, 1}, {0, 0, 0}, scale};
    }

    friend QuatPosScale operator*(const QuatPos& arg0, const QuatPosScale& arg1) {
        return {arg0.quat * arg1.quat, arg0.quat * arg1.pos + arg0.pos, arg1.scale};
    }

    Float3 operator*(const Float3& p) const {
        return quat * (p * scale) + pos;
    }

    Float4x4 to_float4x4() const {
        return Float4x4::from_quaternion(quat, pos) * Float4x4::make_scale(scale);
    }

    Float3x4 to_float3x4() const {
        return Float3x4::from_quaternion(quat, pos) * Float3x4::make_scale(scale);
    }

    QuatPos& as_quat_pos() {
        return (QuatPos&) *this;
    }

    const QuatPos& as_quat_pos() const {
        return (const QuatPos&) *this;
    }
};

inline QuatPos mix(const QuatPos& lo, const QuatPos& hi, float f) {
    QuatPos r;
    r.quat = mix(lo.quat, hi.quat, f);
    r.pos = mix(lo.pos, hi.pos, f);
    return r;
}

inline QuatPosScale mix(const QuatPosScale& lo, const QuatPosScale& hi, float f) {
    QuatPosScale r;
    r.quat = mix(lo.quat, hi.quat, f);
    r.pos = mix(lo.pos, hi.pos, f);
    r.scale = mix(lo.scale, hi.scale, f);
    return r;
}

} // namespace ply
