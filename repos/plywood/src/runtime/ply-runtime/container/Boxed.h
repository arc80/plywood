/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/io/InStream.h>
#include <ply-runtime/io/OutStream.h>
#include <ply-runtime/string/String.h>
#include <ply-runtime/container/Buffer.h>

namespace ply {

//-----------------------------------------------------
// Boxed I/O
//-----------------------------------------------------
template <typename T>
struct Boxed {};

template <>
struct Boxed<String> {
    static void write(NativeEndianWriter& wr, const StringView view) {
        wr.write(view.numBytes);
        wr.outs->write(view.bufferView());
    }

    static PLY_NO_INLINE String read(NativeEndianReader& rd) {
        u32 numBytes = rd.read<u32>();
        if (rd.ins->atEOF())
            return {};
        if (numBytes >= 0x60000000)
            PLY_DEBUG_BREAK();
        auto s = String::allocate(numBytes);
        rd.ins->read(s.bufferView());
        return s;
    }
};

template <>
struct Boxed<Buffer> {
    static void write(NativeEndianWriter& wr, const ConstBufferView view) {
        wr.write(view.numBytes);
        wr.outs->write(view);
    }

    static Buffer read(NativeEndianReader& rd);
};

} // namespace ply
