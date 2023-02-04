/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-runtime/Core.h>

#if !defined(PLY_IMPL_LOGGER_PATH)
#if PLY_TARGET_WIN32
#define PLY_IMPL_LOGGER_PATH "impl/Logger_Win32.h"
#define PLY_IMPL_LOGGER_TYPE Logger_Win32
#elif PLY_TARGET_POSIX
#define PLY_IMPL_LOGGER_PATH "impl/Logger_Stdout.h"
#define PLY_IMPL_LOGGER_TYPE Logger_Stdout
#else
#define PLY_IMPL_LOGGER_PATH "*** Unable to select a default Logger implementation ***"
#endif
#endif

#include PLY_IMPL_LOGGER_PATH

namespace ply {

using Logger = PLY_IMPL_LOGGER_TYPE;

} // namespace ply
