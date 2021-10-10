/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>

namespace ply {

struct DateTime {
    s32 year = 0;
    u8 month = 0;
    u8 day = 0;
    u8 weekday = 0; // [0, 6] -> [Sun, Sat]
    u8 hour = 0;
    u8 minute = 0;
    u8 second = 0;
    s8 timeZoneHour = 0;
    u8 timeZoneMinute = 0;
    u32 microseconds = 0;

    // Number of microseconds since January 1, 1970 at 00:00:00 UTC.
    static PLY_DLL_ENTRY s64 getCurrentEpochMicroseconds();

    // Conversion
    static PLY_DLL_ENTRY DateTime fromEpochMicroseconds(s64 us);
    PLY_DLL_ENTRY s64 toEpochMicroseconds() const;
};

} // namespace ply
