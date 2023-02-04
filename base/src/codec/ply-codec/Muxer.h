/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-codec/Core.h>
#include <image/Image.h>
#include <audio-primitives/Buffer.h>

namespace ply {

struct YUVImage {
    image::Image planes[3];
};

class Muxer {
public:
    float sampleRate = 0;

    virtual ~Muxer() {
    }

    virtual void beginVideoFrame(image::Image& rgbIm) = 0;
    virtual void endVideoFrame() = 0;
    virtual double getVideoTime() = 0;
    virtual u32 getVideoFrameNumber() = 0;
    virtual void flushVideo() = 0;

    virtual void beginAudioFrame(audio::Buffer& buffer) = 0;
    virtual void endAudioFrame(u32 numSamples) = 0;
    virtual double getAudioTime() = 0;
    virtual void flushAudio() = 0;
};

struct VideoOptions {
    // FIXME: Add more options
    u32 width = 1280;
    u32 height = 720;
    u64 bitRate = 1000000;
    // default is 30fps

    VideoOptions(u32 width, u32 height, u64 bitRate = 1000000)
        : width{width}, height{height}, bitRate{bitRate} {
    }
};

struct AudioOptions {
    u64 bitRate = 128000;
    u32 sampleRate = 44100;
    u32 numChannels = 1;
};

Owned<Muxer> createMuxer(OutPipe* out, const VideoOptions* videoOpts, const AudioOptions* audioOpts,
                         StringView containerFormat);

} // namespace ply
