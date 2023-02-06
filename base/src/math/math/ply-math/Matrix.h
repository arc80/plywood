/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-math/Core.h>
#include <ply-math/Vector.h>
#include <ply-math/Quaternion.h>
#include <ply-math/Complex.h>
#include <ply-math/Box.h>
#include <initializer_list>

namespace ply {

struct Float2x2;
struct Float3x3;
struct Float3x4;
struct Float4x4;
struct QuatPos;

//------------------------------------------------------------------------------------------------
/*!
A 2x2 matrix of floating-point values.

A `Float2x2` can represent any linear transformation on the 2D plane without a
translation component, such as rotation, scale or shear.

The matrix is stored in column-major order. In other words, it's an array of two
`Float2` column vectors.
*/
struct Float2x2 {
    Float2 col[2];

    /*!
    \category Constructors
    Constructs an uninitialized `Float2x2`.
    */
    PLY_INLINE Float2x2() = default;
    /*!
    Constructs a 2x2 matrix from the given column vectors.

        Float2x2 m = {{1, 0}, {0, 1}};
    */
    PLY_INLINE Float2x2(const Float2& col0, const Float2& col1) : col{col0, col1} {
    }
    /*!
    \category Element Access
    \begin_group
    Accesses the column vector at the specified index.

        Float2x2 m = {{1, 0}, {0, 1}};
        m[0].x = -1;
        Console::out() << m[1];  // "{0, 1}"
    */
    PLY_INLINE Float2& operator[](ureg i) {
        PLY_ASSERT(i < 2);
        return col[i];
    }
    PLY_INLINE const Float2& operator[](ureg i) const {
        PLY_ASSERT(i < 2);
        return col[i];
    }
    /*!
    \end_group
    */
    /*!
    \category Comparison Functions
    \category Creation Functions
    Returns the identity matrix `{{1, 0}, {0, 1}}`.
    */
    static Float2x2 identity();
    /*!
    Returns a scale matrix.
    */
    static Float2x2 make_scale(const Float2& scale);
    /*!
    Returns a counter-clockwise rotation matrix. The angle is specified in radians.
    */
    static Float2x2 make_rotation(float radians);
    /*!
    Returns a rotation and scale matrix whose first column is given by `c`. Transforming
    a vector by this matrix is equivalent to premultiplying by the vector by `c` on the
    complex plane.

        Float2x2::from_complex({1, 0})  // returns the identity matrix
    */
    static Float2x2 from_complex(const Float2& c);
    /*!
    \category Matrix Operations
    Returns the transpose of the 2x2 matrix. If the matrix is
    [orthogonal](https://en.wikipedia.org/wiki/Orthogonal_matrix) (in other words, it
    consists only of a rotation and/or reflection), this function also returns the
    inverse matrix.
    */
    Float2x2 transposed() const;
};

/*!
\add_to_class Float2x2
\category Comparison Functions
\begin_group
Returns `true` if the matrices are equal (or not equal) using floating-point comparison.
In particular, a component with a value of `0.f` is equal to a component with a value of
`-0.f`.
*/
bool operator==(const Float2x2& a, const Float2x2& b);
PLY_INLINE bool operator!=(const Float2x2& a, const Float2x2& b) {
    return !(a == b);
}
/*!
\end_group
*/
/*!
\category Matrix Operations
Transform a vector using a matrix. `v` is treated as a column vector and premultiplied
by the matrix `m`.
*/
Float2 operator*(const Float2x2& m, const Float2& v);
void operator*(const Float2x2&, float) = delete; // not sure whether to support this
/*!
Matrix multiplication.
*/
Float2x2 operator*(const Float2x2& a, const Float2x2& b);

//------------------------------------------------------------------------------------------------
/*!
A 3x3 matrix of floating-point values.

A `Float3x3` can represent any linear transformation in 3D space without a translation
component, such as rotation, scale or shear. If a translation component is needed, use
`Float3x4` or `Float4x4`.

The matrix is stored in column-major order. In other words, it's an array of three
`Float3` column vectors.
*/
struct Float3x3 {
    Float3 col[3];

    /*!
    \category Constructors
    Constructs an uninitialized `Float3x3`.
    */
    PLY_INLINE Float3x3() = default;
    /*!
    Constructs a 3x3 matrix from the given column vectors.

        Float3x3 m = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    */
    PLY_INLINE Float3x3(const Float3& col0, const Float3& col1, const Float3& col2)
        : col{col0, col1, col2} {
    }
    /*!
    \category Element Access
    \begin_group
    Accesses the column vector at the specified index.

        Float3x3 m = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
        m[0].x = -1;
        Console::out() << m[2];  // "{0, 0, 1}"
    */
    PLY_INLINE Float3& operator[](ureg i) {
        PLY_ASSERT(i < 3);
        return col[i];
    }
    PLY_INLINE const Float3& operator[](ureg i) const {
        PLY_ASSERT(i < 3);
        return col[i];
    }
    /*!
    \end_group
    */
    /*!
    \category Comparison Functions
    \category Creation Functions
    Returns the identity matrix `{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}`.
    */
    static Float3x3 identity();
    /*!
    Returns a scale matrix.
    */
    static Float3x3 make_scale(const Float3& arg);
    /*!
    Returns a matrix that performs a counter-clockwise rotation around the specified
    axis following the [right-hand
    rule](https://en.wikipedia.org/wiki/Right-hand_rule#Rotations). `unit_axis` must
    have have unit length and the angle is specified in radians.
    */
    static Float3x3 make_rotation(const Float3& unit_axis, float radians);
    /*!
    Returns a matrix that performs the same rotation as `q`.
    */
    static Float3x3 from_quaternion(const Quaternion& q);

    bool has_scale(float thresh = 0.001f) const;
    /*!
    \category Matrix Operations
    Returns the transpose of the 3x3 matrix. If the matrix is
    [orthogonal](https://en.wikipedia.org/wiki/Orthogonal_matrix) (in other words, it
    consists only of a rotation and/or reflection), this function also returns the
    inverse matrix.
    */
    Float3x3 transposed() const;
};

/*!
\add_to_class Float3x3
\category Comparison Functions
\begin_group
Returns `true` if the matrices are equal (or not equal) using floating-point comparison.
In particular, a component with a value of `0.f` is equal to a component with a value of
`-0.f`.
*/
bool operator==(const Float3x3& a, const Float3x3& b);
PLY_INLINE bool operator!=(const Float3x3& a, const Float3x3& b) {
    return !(a == b);
}
/*!
\end_group
*/
/*!
\category Matrix Operations
Transform a vector using a matrix. `v` is treated as a column vector and premultiplied
by the matrix `m`.
*/
Float3 operator*(const Float3x3& m, const Float3& v);
void operator*(const Float3x3&, float) = delete; // not sure whether to support this
/*!
Matrix multiplication.
*/
Float3x3 operator*(const Float3x3& a, const Float3x3& b);

//------------------------------------------------------------------------------------------------
/*!
A 3x4 matrix of floating-point values having 3 rows and 4 columns.

A `Float3x4` can represent any affine transformation in 3D space including rotations,
scales, shears and translations.

The matrix is stored in column-major order. In other words, it's an array of four
`Float3` column vectors.

In general, a `Float3x4` acts like a `Float4x4` with **[0 0 0 1]** as the implicit
fourth row. The main difference between `Float3x4` and `Float4x4` is that `Float3x4`
can't represent a perspective projection matrix.
*/
struct Float3x4 {
    Float3 col[4];

    /*!
    \category Constructors
    Constructs an uninitialized `Float3x4`.
    */
    PLY_INLINE Float3x4() = default;
    /*!
    Constructs a 3x4 matrix from the given column vectors.

        Float3x3 m = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {0, 0, 0}};
    */
    PLY_INLINE Float3x4(const Float3& col0, const Float3& col1, const Float3& col2,
                        const Float3& col3)
        : col{col0, col1, col2, col3} {
    }
    /*!
    Constructs a 3x4 matrix from a 3x3 matrix and optional fourth column vector. When
    the resulting 3x4 matrix transforms a `Float3`, it's equivalent to a transformation
    by `m3x3` followed by a translation by `pos`.

        Float4x4 m = {Float3x3::identity(), {5, 0, 0}};
    */
    explicit Float3x4(const Float3x3& m3x3, const Float3& pos = {0, 0, 0});
    /*!
    \category Conversion Functions
    Returns a const reference to the first three columns as `Float3x3` using type
    punning. This should only be used as a temporary expression.
    */
    const Float3x3& as_float3x3() const {
        return reinterpret_cast<const Float3x3&>(*this);
    }
    /*!
    Returns a 4x4 matrix by adding **[0 0 0 1]** as the fourth row.
    */
    Float4x4 to_float4x4() const;
    /*!
    \category Element Access
    \begin_group
    Accesses the column vector at the specified index. In general, `m[3]` can be thought
    of as the translation component.

        Float3x4 m = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {0, 0, 0}};
        m[0].x = -1;
        Console::out() << m[3];  // "{0, 0, 0}"
    */
    Float3& operator[](ureg i) {
        PLY_ASSERT(i < 4);
        return col[i];
    }
    const Float3& operator[](ureg i) const {
        PLY_ASSERT(i < 4);
        return col[i];
    }
    /*!
    \end_group
    */
    /*!
    \category Comparison Functions
    \category Creation Functions
    Returns the identity matrix `{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {0, 0, 0}}`.
    */
    static Float3x4 identity();
    /*!
    Returns a scale matrix.
    */
    static Float3x4 make_scale(const Float3& arg);
    /*!
    Returns a matrix that performs a counter-clockwise rotation around the specified
    axis following the [right-hand
    rule](https://en.wikipedia.org/wiki/Right-hand_rule#Rotations). `unit_axis` must
    have have unit length and the angle is specified in radians.
    */
    static Float3x4 make_rotation(const Float3& unit_axis, float radians);
    /*!
    Returns a translation matrix.
    */
    static Float3x4 make_translation(const Float3& pos);
    /*!
    Returns a matrix that performs the same rotation as `q`. If `pos` is specified, it's
    used as the fourth column, so that the resulting matrix performs the same rotation
    as `q` followed by a translation by `pos`.
    */
    static Float3x4 from_quaternion(const Quaternion& q, const Float3& pos = {0, 0, 0});
    /*!
    Returns a matrix that performs the same transformation as `qp`.
    */
    static Float3x4 from_quat_pos(const QuatPos& qp);

    bool has_scale(float thresh = 0.001f) const;
    /*!
    \category Matrix Operations
    A fast method to compute the inverse of a 3x4 matrix whose first three columns are
    [orthogonal](https://en.wikipedia.org/wiki/Orthogonal_matrix). In other words, the
    input matrix must only consist of a rotation, translation and/or reflection; no
    scale or shear is allowed.
    */
    Float3x4 inverted_ortho() const;
};

/*!
\add_to_class Float3x4
\category Comparison Functions
\begin_group
Returns `true` if the matrices are equal (or not equal) using floating-point comparison.
In particular, a component with a value of `0.f` is equal to a component with a value of
`-0.f`.
*/
bool operator==(const Float3x4& a, const Float3x4& b);
PLY_INLINE bool operator!=(const Float3x4& a, const Float3x4& b) {
    return !(a == b);
}
/*!
\end_group
*/
/*!
\category Matrix Operations
Transform the vector `v` by the 3x4 matrix `m`. `v` is interpreted as a column vector
with an implicit fourth component equal to 1 and premultiplied by `m`. This
transformation is equivalent to transforming `v` by the first three columns of `m`, then
translating the result by the fourth column, as in `m.as_float3x3() * v + m[3]`.

If you don't want the implicit fourth component equal to 1, so that no translation is
performed, use `m.as_float3x3() * v` instead.
*/
Float3 operator*(const Float3x4& m, const Float3& v);
/*!
Interpret `m` as a 4x4 matrix with **[0 0 0 1]** as the fourth row and use it to
transform the vector `v`.
*/
Float4 operator*(const Float3x4& m, const Float4& v);
void operator*(const Float3x4&, float) = delete; // not sure whether to support this
/*!
Matrix multiplication. All matrices are interpreted as 4x4 matrices with **[0 0 0 1]**
as the implicit fourth row.
*/
Float3x4 operator*(const Float3x4& a, const Float3x4& b);

PLY_INLINE static Quaternion from_ortho(const Float3x4& m) {
    return Quaternion::from_ortho(m.as_float3x3());
}

//------------------------------------------------------------------------------------------------
/*!
A 4x4 matrix of floating-point values.

A `Float4x4` can represent any affine transformation in 3D space including rotations,
scales, shears and translations. Unlike `Float3x4`, it can also represent perspective
projections.

The matrix is stored in column-major order. In other words, it's an array of four
`Float4` column vectors.
*/
struct Float4x4 {
    Float4 col[4];

    /*!
    \category Constructors
    Constructs an uninitialized `Float4x4`.
    */
    Float4x4() = default;
    /*!
    Constructs a 4x4 matrix from the given column vectors.

        Float4x4 m = {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}};
    */
    Float4x4(const Float4& col0, const Float4& col1, const Float4& col2,
             const Float4& col3)
        : col{col0, col1, col2, col3} {
    }
    /*!
    Constructs a 4x4 matrix by concatenating a 3x3 matrix with an optional fourth column
    vector and adding **[0 0 0 1]** as the fourth row. When the resulting 4x4 matrix
    transforms a `Float3`, it's equivalent to a transformation by `m3x3` followed by a
    translation by `pos`.

        Float4x4 m = {Float3x3::identity(), {5, 0, 0}};
    */
    explicit Float4x4(const Float3x3& m3x3, const Float3& pos = {0, 0, 0});
    /*!
    \category Conversion Functions
    Returns a 3x3 matrix by truncating the fourth column and fourth row of the 4x4
    matrix.
    */
    Float3x3 to_float3x3() const;
    /*!
    Returns a 3x4 matrix by truncating the fourth row of the 4x4 matrix.
    */
    Float3x4 to_float3x4() const;
    /*!
    \category Element Access
    \begin_group
    Accesses the column vector at the specified index.

        Float4x4 m = {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}};
        m[0].x = -1;
        Console::out() << m[3];  // "{0, 0, 0, 1}"
    */
    Float4& operator[](ureg i) {
        PLY_ASSERT(i < 4);
        return col[i];
    }
    const Float4& operator[](ureg i) const {
        PLY_ASSERT(i < 4);
        return col[i];
    }
    /*!
    \end_group
    */
    /*!
    \category Comparison Functions
    \category Creation Functions
    Returns the identity matrix `{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0,
    1}}`.
    */
    static Float4x4 identity();
    /*!
    Returns a scale matrix.
    */
    static Float4x4 make_scale(const Float3& arg);
    /*!
    Returns a matrix that performs a counter-clockwise rotation around the specified
    axis following the [right-hand
    rule](https://en.wikipedia.org/wiki/Right-hand_rule#Rotations). `unit_axis` must
    have have unit length and the angle is specified in radians.
    */
    static Float4x4 make_rotation(const Float3& unit_axis, float radians);
    /*!
    Returns a translation matrix.
    */
    static Float4x4 make_translation(const Float3& pos);
    /*!
    Returns a matrix that performs the same rotation as `q`. If `pos` is specified, it's
    used as the fourth column, so that the resulting matrix performs the same rotation
    as `q` followed by a translation by `pos`.
    */
    static Float4x4 from_quaternion(const Quaternion& q, const Float3& pos = {0, 0, 0});
    /*!
    Returns a matrix that performs the same transformation as `qp`.
    */
    static PLY_INLINE Float4x4 from_quat_pos(const QuatPos& qp);
    /*!
    \begin_group
    Returns a perspective projection matrix that maps a 3D frustum to OpenGL clip space
    in homogeneous coordinates. OpenGL clip space goes from -1 at the near plane to +1
    at the far plane, with x and y coordinates in the range [-1, 1].

    Assuming a right-handed coordinate system, the 3D frustum starts at the origin,
    faces the -z direction, and intersects the z = -1 plane at the rectangle given by
    `frustum`. Use the `rect_from_fov()` helper function to convert a field-of-view
    angle and aspect ratio to a `frustum` rectangle.

        Float4x4 p = Float4x4::make_projection(rect_from_fov(Pi / 2, 16.f / 9), 1, 100);

    `z_near` and `z_far` are interpreted as distances from the origin and must be
    positive values. Since the 3D frustum faces the -z direction, the actual near and
    far clip planes are given by z = `-z_near` and z = `-z_far`.

    Note: This type of projection matrix works, but is considered outdated. More modern
    projection matrices, such as those [using reversed z or having an infinite far
    plane](https://developer.nvidia.com/content/depth-precision-visualized), typically
    offer better precision, but those aren't implemented in Plywood yet.
    */
    static Float4x4 make_projection(const Rect& frustum, float z_near, float z_far);
    /*!
    \end_group
    */
    /*!
    Returns an orthographic projection matrix that maps a 3D box to OpenGL clip space.
    OpenGL clip space goes from -1 at the near plane to +1 at the far plane, with x and
    y coordinates in the range [-1, 1].

    Assuming a right-handed coordinate system, the 3D box being transformed faces the -z
    direction. `z_near` and `z_far` are interpreted as distances from the origin, so the
    actual near and far clip planes are given by z = `-z_near` and z = `-z_far`.
    */
    static Float4x4 make_ortho(const Rect& rect, float z_near, float z_far);
    /*!
    \category Matrix Operations
    Returns the transpose of the 4x4 matrix.
    */
    Float4x4 transposed() const;
    /*!
    A fast method to compute the inverse of a 4x4 matrix whose fourth row is **[0 0 0
    1]** and whose first three columns are
    [orthogonal](https://en.wikipedia.org/wiki/Orthogonal_matrix). In other words, the
    input matrix must only consist of a rotation, translation and/or reflection; no
    scale or shear is allowed.
    */
    Float4x4 inverted_ortho() const;
};

/*!
\add_to_class Float4x4
\category Comparison Functions
\begin_group
Returns `true` if the matrices are equal (or not equal) using floating-point comparison.
In particular, a component with a value of `0.f` is equal to a component with a value of
`-0.f`.
*/
bool operator==(const Float4x4& a, const Float4x4& b);
PLY_INLINE bool operator!=(const Float4x4& a, const Float4x4& b) {
    return !(a == b);
}
/*!
\end_group
*/
/*!
\category Matrix Operations
Transform a vector using a matrix. `v` is treated as a column vector and premultiplied
by the matrix `m`.
*/
Float4 operator*(const Float4x4& m, const Float4& v);
void operator*(const Float4x4&, float) = delete; // not sure whether to support this
/*!
\begin_group
Matrix multiplication. `Float3x4` arguments are interpreted as 4x4 matrices with **[0 0
0 1]** as the implicit fourth row.
*/
Float4x4 operator*(const Float4x4& a, const Float4x4& b);
Float4x4 operator*(const Float3x4& a, const Float4x4& b);
Float4x4 operator*(const Float4x4& a, const Float3x4& b);
/*!
\end_group
*/

} // namespace ply
