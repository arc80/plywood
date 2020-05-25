/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-math/Core.h>
#include <ply-reflect/TypeDescriptor.h>

namespace ply {

struct Float2;
struct Float3;
struct Float4;
struct Quaternion;
struct Float2x2;
struct Float3x3;
struct Float3x4;
struct Float4x4;
template <typename>
struct Int2;
template <typename>
struct Int3;
template <typename>
struct Box;
struct QuatPos;
struct QuatPosScale;
enum class Axis3 : u8;
enum class Axis2 : u8;
struct AxisRot;
struct AxisRotPos;

PLY_DECLARE_TYPE_DESCRIPTOR(PLY_DLL_ENTRY, , Float2)
PLY_DECLARE_TYPE_DESCRIPTOR(PLY_DLL_ENTRY, , Float3)
PLY_DECLARE_TYPE_DESCRIPTOR(PLY_DLL_ENTRY, , Float4)
PLY_DECLARE_TYPE_DESCRIPTOR(PLY_DLL_ENTRY, , Quaternion)
PLY_DECLARE_TYPE_DESCRIPTOR(PLY_DLL_ENTRY, , Float2x2)
PLY_DECLARE_TYPE_DESCRIPTOR(PLY_DLL_ENTRY, , Float3x3)
PLY_DECLARE_TYPE_DESCRIPTOR(PLY_DLL_ENTRY, , Float3x4)
PLY_DECLARE_TYPE_DESCRIPTOR(PLY_DLL_ENTRY, , Float4x4)
PLY_DECLARE_TYPE_DESCRIPTOR(PLY_DLL_ENTRY, , Box<Int2<u16>>)
PLY_DECLARE_TYPE_DESCRIPTOR(PLY_DLL_ENTRY, , Box<Float2>)
PLY_DECLARE_TYPE_DESCRIPTOR(PLY_DLL_ENTRY, , Box<Int2<s16>>)
PLY_DECLARE_TYPE_DESCRIPTOR(PLY_DLL_ENTRY, , Int2<u16>)
PLY_DECLARE_TYPE_DESCRIPTOR(PLY_DLL_ENTRY, , Int2<s16>)
PLY_DECLARE_TYPE_DESCRIPTOR(PLY_DLL_ENTRY, , Int3<u8>)
PLY_DECLARE_TYPE_DESCRIPTOR(PLY_DLL_ENTRY, _Struct, QuatPos)
PLY_DECLARE_TYPE_DESCRIPTOR(PLY_DLL_ENTRY, _Struct, QuatPosScale)
PLY_DECLARE_TYPE_DESCRIPTOR(PLY_DLL_ENTRY, _Enum, Axis3)
PLY_DECLARE_TYPE_DESCRIPTOR(PLY_DLL_ENTRY, _Enum, Axis2)
PLY_DECLARE_TYPE_DESCRIPTOR(PLY_DLL_ENTRY, _Struct, AxisRot)
PLY_DECLARE_TYPE_DESCRIPTOR(PLY_DLL_ENTRY, _Struct, AxisRotPos)

} // namespace ply
