/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-runtime/Base.h>

namespace pylon {
using namespace ply;
} // namespace pylon

#if PYLON_IMPORTING
#define PYLON_ENTRY PLY_DLL_IMPORT
#elif PYLON_EXPORTING
#define PYLON_ENTRY PLY_DLL_EXPORT
#else
#define PYLON_ENTRY
#endif
