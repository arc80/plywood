/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-reflect/Core.h>
#include <ply-reflect/TypeDescriptor_Def.h>

namespace ply {

extern TypeKey TypeKey_S8;
extern TypeKey TypeKey_S16;
extern TypeKey TypeKey_S32;
extern TypeKey TypeKey_S64;
extern TypeKey TypeKey_U8;
extern TypeKey TypeKey_U16;
extern TypeKey TypeKey_U32;
extern TypeKey TypeKey_U64;
extern TypeKey TypeKey_Float;
extern TypeKey TypeKey_Double;

PLY_DECLARE_TYPE_DESCRIPTOR(s8)
PLY_DECLARE_TYPE_DESCRIPTOR(s16)
PLY_DECLARE_TYPE_DESCRIPTOR(s32)
PLY_DECLARE_TYPE_DESCRIPTOR(s64)
PLY_DECLARE_TYPE_DESCRIPTOR(u8)
PLY_DECLARE_TYPE_DESCRIPTOR(u16)
PLY_DECLARE_TYPE_DESCRIPTOR(u32)
PLY_DECLARE_TYPE_DESCRIPTOR(u64)
PLY_DECLARE_TYPE_DESCRIPTOR(float)
PLY_DECLARE_TYPE_DESCRIPTOR(double)

} // namespace ply
