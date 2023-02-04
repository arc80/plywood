/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <image-reflect/Core.h>
#include <image-reflect/ImageReflect.h>

PLY_ENUM_BEGIN(ply::image::, Format)
PLY_ENUM_IDENTIFIER(Unknown)
PLY_ENUM_IDENTIFIER(Char)
PLY_ENUM_IDENTIFIER(Byte)
PLY_ENUM_IDENTIFIER(Byte2)
PLY_ENUM_IDENTIFIER(RGBA)
PLY_ENUM_IDENTIFIER(BGRA)
PLY_ENUM_IDENTIFIER(ARGB)
PLY_ENUM_IDENTIFIER(ABGR)
PLY_ENUM_IDENTIFIER(Float)
PLY_ENUM_IDENTIFIER(S16)
PLY_ENUM_IDENTIFIER(U16)
PLY_ENUM_IDENTIFIER(S16_2)
PLY_ENUM_IDENTIFIER(Half)
PLY_ENUM_IDENTIFIER(Float2)
PLY_ENUM_IDENTIFIER(Half2)
PLY_ENUM_IDENTIFIER(Float4)
PLY_ENUM_IDENTIFIER(Half4)
PLY_ENUM_IDENTIFIER(D24S8)
PLY_ENUM_END()
