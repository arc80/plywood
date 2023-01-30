/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/io/InStream.h>
#include <ply-runtime/io/OutStream.h>
#include <ply-runtime/string/String.h>

namespace ply {

//-----------------------------------------------------
// Boxed I/O
//-----------------------------------------------------
template <typename T>
struct Boxed {};

template <>
struct Boxed<String> {
    static void write(NativeEndianWriter& wr, StringView view) {
        wr.write(view.numBytes);
        wr.out << view;
    }

    static PLY_NO_INLINE String read(NativeEndianReader& rd);
};

} // namespace ply
