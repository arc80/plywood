/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/string/StringView.h>

namespace ply {

class Logger_Win32 {
public:
    static void log(StringView strWithOptionalNullTerminator) {
        OutputDebugStringA((LPCSTR) strWithOptionalNullTerminator.withNullTerminator().bytes);
    }
};

} // namespace ply
