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

PLY_DECLARE_TYPE_DESCRIPTOR(Float2, PLY_DLL_ENTRY)
PLY_DECLARE_TYPE_DESCRIPTOR(Float3, PLY_DLL_ENTRY)
PLY_DECLARE_TYPE_DESCRIPTOR(Float4, PLY_DLL_ENTRY)
PLY_DECLARE_TYPE_DESCRIPTOR(Quaternion, PLY_DLL_ENTRY)
PLY_DECLARE_TYPE_DESCRIPTOR(Float2x2, PLY_DLL_ENTRY)
PLY_DECLARE_TYPE_DESCRIPTOR(Float3x3, PLY_DLL_ENTRY)
PLY_DECLARE_TYPE_DESCRIPTOR(Float3x4, PLY_DLL_ENTRY)
PLY_DECLARE_TYPE_DESCRIPTOR(Float4x4, PLY_DLL_ENTRY)
PLY_DECLARE_TYPE_DESCRIPTOR(Box<Int2<u16>>, PLY_DLL_ENTRY)
PLY_DECLARE_TYPE_DESCRIPTOR(Box<Float2>, PLY_DLL_ENTRY)
PLY_DECLARE_TYPE_DESCRIPTOR(Box<Int2<s16>>, PLY_DLL_ENTRY)
PLY_DECLARE_TYPE_DESCRIPTOR(Int2<u16>, PLY_DLL_ENTRY)
PLY_DECLARE_TYPE_DESCRIPTOR(Int2<s16>, PLY_DLL_ENTRY)
PLY_DECLARE_TYPE_DESCRIPTOR(Int3<u8>, PLY_DLL_ENTRY)
PLY_DECLARE_TYPE_DESCRIPTOR(QuatPos, PLY_DLL_ENTRY)
PLY_DECLARE_TYPE_DESCRIPTOR(QuatPosScale, PLY_DLL_ENTRY)
PLY_DECLARE_TYPE_DESCRIPTOR(Axis3, PLY_DLL_ENTRY)
PLY_DECLARE_TYPE_DESCRIPTOR(Axis2, PLY_DLL_ENTRY)
PLY_DECLARE_TYPE_DESCRIPTOR(AxisRot, PLY_DLL_ENTRY)
PLY_DECLARE_TYPE_DESCRIPTOR(AxisRotPos, PLY_DLL_ENTRY)

} // namespace ply
