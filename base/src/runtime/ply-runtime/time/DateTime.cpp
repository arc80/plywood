/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime.h>

#if !PLY_DLL_IMPORTING

namespace ply {

#if PLY_TARGET_WIN32

s64 DateTime::get_current_epoch_microseconds() {
    FILETIME file_time;
    ULARGE_INTEGER large_integer;

    GetSystemTimeAsFileTime(&file_time);
    large_integer.LowPart = file_time.dwLowDateTime;
    large_integer.HighPart = file_time.dwHighDateTime;
    return s64(large_integer.QuadPart / 10) - 11644473600000000ll;
}

#elif PLY_TARGET_POSIX

#if PLY_USE_POSIX_2008_CLOCK
#include <time.h>
#else
#include <sys/time.h>
#endif

s64 DateTime::get_current_epoch_microseconds() {
#if PLY_USE_POSIX_2008_CLOCK
    struct timespec tick;
    clock_gettime(CLOCK_REALTIME, &tick);
    return (s64) tick.tv_sec * 1000000 + tick.tv_nsec / 1000;
#else
    struct timeval tick;
    gettimeofday(&tick, NULL);
    return (s64) tick.tv_sec * 1000000 + tick.tv_usec;
#endif
}

#endif

// FIXME: Move this to runtime/Core.h?
template <typename T>
PLY_INLINE T floor_div(T dividend, T divisor) {
    PLY_STATIC_ASSERT(std::is_arithmetic<T>::value);
    PLY_ASSERT(divisor > 0);
    if (dividend < 0) {
        dividend += divisor - 1;
    }
    return dividend / divisor;
}

// Based on http://howardhinnant.github.io/date_algorithms.html
void set_date_from_epoch_days(DateTime* date_time, s32 days) {
    days += 719468;
    s32 era = (days >= 0 ? days : days - 146096) / 146097;
    u32 doe = u32(days - era * 146097);                              // [0, 146096]
    u32 yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; // [0, 399]
    s32 y = s32(yoe) + era * 400;
    u32 doy = doe - (365 * yoe + yoe / 4 - yoe / 100); // [0, 365]
    u32 mp = (5 * doy + 2) / 153;                      // [0, 11]
    u32 d = doy - (153 * mp + 2) / 5 + 1;              // [1, 31]
    u32 m = mp + (mp < 10 ? 3 : -9);                   // [1, 12]
    date_time->year = y + (m <= 2);
    date_time->month = safe_demote<u8>(m);
    date_time->day = safe_demote<u8>(d);
    date_time->weekday =
        safe_demote<u8>(days >= -4 ? (days + 4) % 7 : (days + 5) % 7 + 6);
}

// Based on http://howardhinnant.github.io/date_algorithms.html
s32 get_epoch_days_from_date(const DateTime& date_time) {
    s32 m = date_time.month;
    s32 y = date_time.year - (m <= 2);
    s32 era = (y >= 0 ? y : y - 399) / 400;
    u32 yoe = u32(y - era * 400);                                          // [0, 399]
    u32 doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + date_time.day - 1; // [0, 365]
    u32 doe = yoe * 365 + yoe / 4 - yoe / 100 + doy; // [0, 146096]
    return era * 146097 + doe - 719468;
}

DateTime DateTime::from_epoch_microseconds(s64 us) {
    static constexpr s64 us_per_day = 86400000000ll;
    s32 days = safe_demote<s32>(floor_div(us, us_per_day));
    u64 us_in_day = safe_demote<u64>(us - (days * us_per_day));

    DateTime date_time;
    set_date_from_epoch_days(&date_time, days);
    u32 secs = safe_demote<u32>(us_in_day / 1000000);
    u32 minutes = secs / 60;
    u32 hours = minutes / 60;
    date_time.hour = safe_demote<u8>(hours);
    date_time.minute = safe_demote<u8>(minutes - hours * 60);
    date_time.second = safe_demote<u8>(secs - minutes * 60);
    date_time.microseconds = safe_demote<u32>(us_in_day - u64(secs) * 1000000);
    return date_time;
}

s64 DateTime::to_epoch_microseconds() const {
    static constexpr s64 us_per_day = 86400000000ll;
    s32 days = get_epoch_days_from_date(*this);
    s32 minutes = s32(this->hour) * 60 + this->minute;
    s32 seconds = minutes * 60 + this->second;
    return s64(days) * us_per_day + s64(seconds) * 1000000 + this->microseconds;
}

} // namespace ply

#endif // !PLY_DLL_IMPORTING
