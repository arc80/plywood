/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
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

    static ScalePos2 makeScale(const Float2& scale) {
        return {scale, 0};
    }

    static ScalePos2 makeTranslation(const Float2& pos) {
        return {{0, 0}, pos};
    }

    static ScalePos2 mapToRect(const Rect& r) {
        return {r.size(), r.mins};
    }

    ScalePos2 operator*(const ScalePos2& arg) const {
        return {scale * arg.scale, (scale * arg.pos) + pos};
    }

    Float2 operator*(const Float2& p) const {
        return (scale * p) + pos;
    }

    ScalePos2 inverted() const {
        Float2 ooScale = 1.f / scale;
        return {ooScale, -pos * ooScale};
    }

    const Float4& asFloat4() const {
        return reinterpret_cast<const Float4&>(*this);
    }
};

inline Rect operator*(const ScalePos2& xform, const Rect& r) {
    return {xform * r.mins, xform * r.maxs};
}

//----------------------------------------------------
// QuatPos
//----------------------------------------------------

struct QuatPos {
    Quaternion quat;
    Float3 pos;

    QuatPos() {
    }

    QuatPos(const Quaternion& quat, const Float3& pos) : quat(quat), pos(pos) {
    }

    static QuatPos identity() {
        return {Quaternion::identity(), {0, 0, 0}};
    }

    static QuatPos fromOrtho(const Float3x4& m) {
        return {Quaternion::fromOrtho(m.asFloat3x3()), m[3]};
    }

    static QuatPos fromOrtho(const Float4x4& m) {
        return {Quaternion::fromOrtho(m), m[3].asFloat3()};
    }

    static QuatPos makeTranslation(const Float3& pos) {
        return {{0, 0, 0, 1}, pos};
    }

    static QuatPos makeRotation(const Float3& unitAxis, float radians) {
        return {Quaternion::fromAxisAngle(unitAxis, radians), {0, 0, 0}};
    }

    QuatPos operator*(const QuatPos& arg) const {
        return {quat * arg.quat, (quat * arg.pos) + pos};
    }

    Float3 operator*(const Float3& p) const {
        return (quat * p) + pos;
    }

    QuatPos operator*(const Quaternion& q) const {
        return {quat * q, pos};
    }

    friend QuatPos operator*(const Quaternion& q, const QuatPos& qp) {
        return {q * qp.quat, q * qp.pos};
    }

    QuatPos inverted() const {
        Quaternion qi = quat.inverted();
        return {qi, qi * -pos};
    }

    Float3x4 toFloat3x4() const {
        return quat.toFloat3x4(pos);
    }

    Float4x4 toFloat4x4() const {
        return quat.toFloat4x4(pos);
    }
};

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

    static QuatPosScale makeTranslation(const Float3& pos) {
        return {{0, 0, 0, 1}, pos, {1, 1, 1}};
    }

    static QuatPosScale makeScale(const Float3& scale) {
        return {{0, 0, 0, 1}, {0, 0, 0}, scale};
    }

    friend QuatPosScale operator*(const QuatPos& arg0, const QuatPosScale& arg1) {
        return {arg0.quat * arg1.quat, arg0.quat * arg1.pos + arg0.pos, arg1.scale};
    }

    Float3 operator*(const Float3& p) const {
        return quat * (p * scale) + pos;
    }

    Float4x4 toFloat4x4() const {
        return quat.toFloat4x4(pos) * Float4x4::makeScale(scale);
    }

    Float3x4 toFloat3x4() const {
        return quat.toFloat3x4(pos) * Float3x4::makeScale(scale);
    }

    QuatPos& asQuatPos() {
        return (QuatPos&) *this;
    }

    const QuatPos& asQuatPos() const {
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
