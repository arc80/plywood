/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-math/Core.h>

#if defined(__arm__) || defined(__arm64__) // FIXME: Better detection
#include <ply-math/neon/Matrix.h>
#else
#include <ply-math/Matrix.h>
#endif

namespace ply {
namespace simd {

#if defined(__arm__) || defined(__arm64__) // FIXME: Better detection
using Float3x3 = neon::Float3x3;
using Float3x4 = neon::Float3x4;
using Float4x4 = neon::Float4x4;
#else
using Float3x3 = Float3x3;
using Float3x4 = Float3x4;
using Float4x4 = Float4x4;
#endif

} // namespace simd
} // namespace ply
