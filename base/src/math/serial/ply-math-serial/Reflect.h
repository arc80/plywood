/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-math.h>
#include <ply-reflect/TypeDescriptor.h>

namespace ply {

struct vec2;
struct vec3;
struct vec4;
struct Quaternion;
struct mat2x2;
struct mat3x3;
struct mat3x4;
struct mat4x4;
template <typename>
struct TVec2;
template <typename>
struct TVec3;
template <typename>
struct Box;
struct QuatPos;
struct QuatPosScale;
enum class Axis : u8;
struct AxisRot;
struct AxisRotPos;

PLY_DECLARE_TYPE_DESCRIPTOR(vec2)
PLY_DECLARE_TYPE_DESCRIPTOR(vec3)
PLY_DECLARE_TYPE_DESCRIPTOR(vec4)
PLY_DECLARE_TYPE_DESCRIPTOR(Quaternion)
PLY_DECLARE_TYPE_DESCRIPTOR(mat2x2)
PLY_DECLARE_TYPE_DESCRIPTOR(mat3x3)
PLY_DECLARE_TYPE_DESCRIPTOR(mat3x4)
PLY_DECLARE_TYPE_DESCRIPTOR(mat4x4)
PLY_DECLARE_TYPE_DESCRIPTOR(Box<TVec2<u16>>)
PLY_DECLARE_TYPE_DESCRIPTOR(Box<vec2>)
PLY_DECLARE_TYPE_DESCRIPTOR(Box<TVec2<s16>>)
PLY_DECLARE_TYPE_DESCRIPTOR(TVec2<u16>)
PLY_DECLARE_TYPE_DESCRIPTOR(TVec2<s16>)
PLY_DECLARE_TYPE_DESCRIPTOR(TVec3<u8>)
PLY_DECLARE_TYPE_DESCRIPTOR(QuatPos)
PLY_DECLARE_TYPE_DESCRIPTOR(QuatPosScale)
PLY_DECLARE_TYPE_DESCRIPTOR(Axis)
PLY_DECLARE_TYPE_DESCRIPTOR(Axis2)
PLY_DECLARE_TYPE_DESCRIPTOR(AxisRot)
PLY_DECLARE_TYPE_DESCRIPTOR(AxisRotPos)

} // namespace ply
