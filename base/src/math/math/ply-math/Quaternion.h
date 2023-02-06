/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-math/Core.h>
#include <ply-math/Vector.h>

namespace ply {

struct Float3x3;
struct Float3x4;
struct Float4x4;

//------------------------------------------------------------------------------------------------
/*!
A quaternion is commonly used to represent a 3D rotation. Like `Float4`, it consists of
four floating-point components `x`, `y`, `z` and `w`.

Only unit quaternions (ie. quaternions with unit length) represent valid 3D rotations.
The `x`, `y` and `z` components give the direction of the rotation axis, while the `w`
component is the cosine of half the rotation angle. Rotation follows the [right-hand
rule](https://en.wikipedia.org/wiki/Right-hand_rule#Rotations) in a right-handed
coordinate system.

When working with quaternions and vectors, the `*` operator behaves differently
depending on the type of the operands:

* If `a` and `b` are both `Quaternion`, `a * b` returns their [Hamilton
  product](https://en.wikipedia.org/wiki/Quaternion#Hamilton_product) (composition).
* If `q` is a unit `Quaternion` and `v` is a `Float3`, `q * v` rotates `v` using `q`.
* If `a` and `b` are both `Float4`, `a * b` performs componentwise multiplication.
*/
struct Quaternion {
    /*!
    \begin_group
    `w` is the fourth component. It follows `z` sequentially in memory.
    */
    float x;
    float y;
    float z;
    float w;
    /*!
    \end_group
    */

    /*!
    \category Constructors
    Constructs an uninitialized `Quaternion`.
    */
    PLY_INLINE Quaternion() = default;
    /*!
    Constructs a `Quaternion` from the given components.

        Quaternion q = {0, 0, 0, 1};
    */
    PLY_INLINE Quaternion(float x, float y, float z, float w) : x{x}, y{y}, z{z}, w{w} {
    }
    /*!
    Constructs a `Quaternion` from a `Float3` and a fourth component.
    */
    PLY_INLINE Quaternion(const Float3& v, float w) : x{v.x}, y{v.y}, z{v.z}, w{w} {
    }
    /*!
    \category Conversion Functions
    Returns a const reference to the first three components as a `Float3` using type
    punning. This should only be used as a temporary expression.
    */
    PLY_INLINE const Float3& as_float3() const {
        PLY_PUN_SCOPE
        return reinterpret_cast<const Float3&>(*this);
    }
    /*!
    Casts to `Float4` using type punning. This should only be used as a temporary
    expression.
    */
    PLY_INLINE const Float4& as_float4() const {
        PLY_PUN_SCOPE
        return reinterpret_cast<const Float4&>(*this);
    }
    /*!
    \category Creation Functions
    Returns the identity quaternion `{0, 0, 0, 1}`.
    */
    static PLY_INLINE Quaternion identity() {
        return {0, 0, 0, 1};
    }
    /*!
    Returns a quaternion that represents a rotation around the given axis. `unit_axis`
    must have unit length. The angle is specified in radians. Rotation follows the
    [right-hand rule](https://en.wikipedia.org/wiki/Right-hand_rule#Rotations) in a
    right-handed coordinate system.
    */
    static Quaternion from_axis_angle(const Float3& unit_axis, float radians);
    /*!
    Returns a unit quaternion that transforms `start` to `end`. Both input vectors must
    have unit length.
    */
    static Quaternion from_unit_vectors(const Float3& start, const Float3& end);
    /*!
    \begin_group
    Converts a rotation matrix to a unit quaternion.
    */
    static Quaternion from_ortho(const Float3x3& m);
    static Quaternion from_ortho(const Float3x4& m);
    static Quaternion from_ortho(const Float4x4& m);
    /*!
    \end_group
    */
    /*!
    \category Rotation Functions
    Given a unit quaternion, returns a unit quaternion that represents its inverse
    rotation.

    This function actually returns the
    [conjugate](https://www.3dgep.com/understanding-quaternions/#Quaternion_Conjugate)
    of the given quaternion by negating its `x`, `y` and `z` components, with the
    understanding that the conjugate of a unit quaternion is also its inverse.
    */
    PLY_INLINE Quaternion inverted() const {
        // Small rotations have large w component, so prefer to keep the same sign of w.
        // Better for interpolation.
        return {-x, -y, -z, w};
    }
    /*!
    \begin_group
    Rotates the special vectors (1, 0, 0), (0, 1, 0) and (0, 0, 1) by the given
    quaternion using fewer instructions than `operator*()`.

    * `q.rotate_unit_x()` is equivalent to `q * Float3{1, 0, 0}`
    * `q.rotate_unit_y()` is equivalent to `q * Float3{0, 1, 0}`
    * `q.rotate_unit_z()` is equivalent to `q * Float3{0, 0, 1}`

    This function will probably be replaced by an overload of `operator*()` that accepts
    `Axis3` values.
    */
    Float3 rotate_unit_x() const;
    Float3 rotate_unit_y() const;
    Float3 rotate_unit_z() const;
    /*!
    \end_group
    */
    /*!
    \category Quaternion Functions
    Returns a unit quaternion having the same direction as `this`. The input quaternion
    must not be zero. When many unit quaternions are composed together, it's a good idea
    to occasionally re-normalize the result to compensate for any drift caused by
    floating-point imprecision.
    */
    PLY_INLINE Quaternion normalized() const {
        return as_float4().normalized().as_quaternion();
    }
    /*!
    \category Interpolation Functions
    Returns either `this` or `-this`, whichever is closer to `other`. If `this` has unit
    length, the resulting quaternion represents the same 3D rotation as `this`.

    In general, when interpolating between arbitrary quaternions `a` and `b`, it's best
    to interpolate from `a` to `b.negated_if_closer_to(a)` since this form takes the
    shortest path.
    */
    Quaternion negated_if_closer_to(const Quaternion& other) const;
};

/*!
\add_to_class Quaternion
\category Quaternion Functions
Unary negation. All components of the original quaternion are negated. If the quaternion
has unit length, the resulting quaternion represents the same 3D rotation as the
original quaternion.
*/
PLY_INLINE Quaternion operator-(const Quaternion& q) {
    return {-q.x, -q.y, -q.z, -q.w};
}
/*!
\category Rotation Functions
Rotates the vector `v` by the quaternion `q`. `q` must have unit length.
*/
Float3 operator*(const Quaternion& q, const Float3& v);
/*!
Composes two quaternions using the [Hamilton
product](https://en.wikipedia.org/wiki/Quaternion#Hamilton_product). If both quaternions
have unit length, the resulting quaternion represents a rotation by `b` followed by a
rotation by `a`.
*/
Quaternion operator*(const Quaternion& a, const Quaternion& b);
/*!
\category Interpolation Functions
Interpolates between two quaternions. Performs a linear interpolation on the components
of `a.negated_if_closer_to(b)` and `b`, then normalizes the result.
*/
Quaternion mix(const Quaternion& a, const Quaternion& b, float f);

PLY_INLINE const Quaternion& Float4::as_quaternion() const {
    PLY_COMPILER_BARRIER();
    return reinterpret_cast<const Quaternion&>(*this);
}

} // namespace ply
