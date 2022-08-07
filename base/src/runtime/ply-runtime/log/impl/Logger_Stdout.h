/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/string/StringView.h>
#if PLY_TARGET_WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

namespace ply {

class Logger_Stdout {
public:
    static void log(StringView strWithOptionalNullTerm) {
        StringView strWithoutNullTerm = strWithOptionalNullTerm.withoutNullTerminator();
#if PLY_TARGET_WIN32
        _write(1, strWithoutNullTerm.bytes, strWithoutNullTerm.numBytes);
#else
        ssize_t rc = ::write(STDOUT_FILENO, strWithoutNullTerm.bytes, strWithoutNullTerm.numBytes);
        PLY_UNUSED(rc);
#endif
    }
};

} // namespace ply
