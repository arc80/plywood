/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-reflect/Core.h>
#include <ply-reflect/TypeDescriptor_Def.h>

namespace ply {

PLY_DLL_ENTRY extern TypeKey TypeKey_S8;
PLY_DLL_ENTRY extern TypeKey TypeKey_S16;
PLY_DLL_ENTRY extern TypeKey TypeKey_S32;
PLY_DLL_ENTRY extern TypeKey TypeKey_S64;
PLY_DLL_ENTRY extern TypeKey TypeKey_U8;
PLY_DLL_ENTRY extern TypeKey TypeKey_U16;
PLY_DLL_ENTRY extern TypeKey TypeKey_U32;
PLY_DLL_ENTRY extern TypeKey TypeKey_U64;
PLY_DLL_ENTRY extern TypeKey TypeKey_Float;
PLY_DLL_ENTRY extern TypeKey TypeKey_Double;

PLY_DECLARE_TYPE_DESCRIPTOR(s8, PLY_DLL_ENTRY)
PLY_DECLARE_TYPE_DESCRIPTOR(s16, PLY_DLL_ENTRY)
PLY_DECLARE_TYPE_DESCRIPTOR(s32, PLY_DLL_ENTRY)
PLY_DECLARE_TYPE_DESCRIPTOR(s64, PLY_DLL_ENTRY)
PLY_DECLARE_TYPE_DESCRIPTOR(u8, PLY_DLL_ENTRY)
PLY_DECLARE_TYPE_DESCRIPTOR(u16, PLY_DLL_ENTRY)
PLY_DECLARE_TYPE_DESCRIPTOR(u32, PLY_DLL_ENTRY)
PLY_DECLARE_TYPE_DESCRIPTOR(u64, PLY_DLL_ENTRY)
PLY_DECLARE_TYPE_DESCRIPTOR(float, PLY_DLL_ENTRY)
PLY_DECLARE_TYPE_DESCRIPTOR(double, PLY_DLL_ENTRY)

} // namespace ply
