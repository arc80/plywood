/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/log/Logger.h>
#include <ply-runtime/io/OutStream.h>
#include <ply-runtime/time/CPUTimer.h>

namespace ply {

template <typename Encoding>
void writeText(OutStream* outs, CPUTimer::Duration duration){};

struct LogChannel {
    StringView channelName;
    static CPUTimer::Point startTime;
    static CPUTimer::Converter converter;

    struct LineHandler {
        MemOutStream mout;
        PLY_DLL_ENTRY LineHandler(StringView channelName);
        PLY_DLL_ENTRY ~LineHandler();
    };

    template <typename... Args>
    PLY_INLINE void log(const char* fmt, const Args&... args) {
        LineHandler lh{channelName};
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
