/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-math/Core.h>
#include <ply-math/Vector.h>
#include <ply-runtime/io/text/StringWriter.h>

namespace ply {
namespace fmt {

template <>
struct TypePrinter<Float2> {
    static PLY_DLL_ENTRY void print(StringWriter* sw, const Float2& v);
};

template <>
struct TypePrinter<Float3> {
    static PLY_DLL_ENTRY void print(StringWriter* sw, const Float3& v);
};

template <>
struct TypePrinter<Float4> {
    static PLY_DLL_ENTRY void print(StringWriter* sw, const Float4& v);
};

} // namespace fmt
} // namespace ply
