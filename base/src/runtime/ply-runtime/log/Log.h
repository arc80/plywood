/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-runtime.h>
#include <ply-runtime/log/Logger.h>

namespace ply {

template <typename Encoding>
void write_text(OutStream& out, CPUTimer::Duration duration){};

struct LogChannel {
    StringView channel_name;
    static CPUTimer::Point start_time;
    static CPUTimer::Converter converter;

    struct LineHandler {
        MemOutStream mout;
        PLY_DLL_ENTRY LineHandler(StringView channel_name);
        PLY_DLL_ENTRY ~LineHandler();
    };

    template <typename... Args>
    PLY_INLINE void log(const char* fmt, const Args&... args) {
        LineHandler lh{channel_name};
        lh.mout.format(fmt, args...);
    }
};

struct LogChannel_Null {
public:
    template <typename... Args>
    void log(const char*, const Args&...) {
    }
};

} // namespace ply

// FIXME: Rename to PLOG
#define SLOG_CHANNEL(channel, name) ply::LogChannel channel{name};
#define SLOG_DECLARE_CHANNEL(channel) extern ply::LogChannel channel;
#define SLOG_NO_CHANNEL(channel) ply::LogChannel_Null channel;
#define SLOG(channel, fmt, ...) (channel).log(fmt, ##__VA_ARGS__)
