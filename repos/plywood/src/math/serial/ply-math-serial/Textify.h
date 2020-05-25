/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-math/Core.h>
#include <ply-math/Vector.h>
#include <ply-runtime/io/text/StringWriter.h>

namespace ply {

template <>
struct Textify<Float4> {
    static void write(OutStream* outs, const Float4& pt) {
        TextWriter{outs}.format("{{{}, {}, {}, {}}}", pt.x, pt.y, pt.z, pt.w);
    }
};

template <>
struct Textify<Float3> {
    static void write(OutStream* outs, const Float3& pt) {
        TextWriter{outs}.format("{{{}, {}, {}}}", pt.x, pt.y, pt.z);
    }
};

template <>
struct Textify<Float2> {
    static void write(OutStream* outs, const Float2& pt) {
        TextWriter{outs}.format("{{{}, {}}}", pt.x, pt.y);
    }
};

} // namespace ply
