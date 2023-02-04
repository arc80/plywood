/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-math/Core.h>

#if defined(__arm__) || defined(__arm64__) // FIXME: Better detection
#include <ply-math/neon/Vector.h>
#else
#include <ply-math/Vector.h>
#endif

namespace ply {
namespace simd {

#if defined(__arm__) || defined(__arm64__) // FIXME: Better detection
using Float3 = neon::Float3;
#else
using Float3 = Float3;
#endif

} // namespace simd
} // namespace ply
