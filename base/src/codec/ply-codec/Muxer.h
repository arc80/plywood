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
    float sample_rate = 0;

    virtual ~Muxer() {
    }

    virtual void begin_video_frame(image::Image& rgb_im) = 0;
    virtual void end_video_frame() = 0;
    virtual double get_video_time() = 0;
    virtual u32 get_video_frame_number() = 0;
    virtual void flush_video() = 0;

    virtual void begin_audio_frame(audio::Buffer& buffer) = 0;
    virtual void end_audio_frame(u32 num_samples) = 0;
    virtual double get_audio_time() = 0;
    virtual void flush_audio() = 0;
};

struct VideoOptions {
    // FIXME: Add more options
    u32 width = 1280;
    u32 height = 720;
    u64 bit_rate = 1000000;
    // default is 30fps

    VideoOptions(u32 width, u32 height, u64 bit_rate = 1000000)
        : width{width}, height{height}, bit_rate{bit_rate} {
    }
};

struct AudioOptions {
    u64 bit_rate = 128000;
    u32 sample_rate = 44100;
    u32 num_channels = 1;
};

Owned<Muxer> create_muxer(OutPipe* out, const VideoOptions* video_opts,
                          const AudioOptions* audio_opts, StringView container_format);

} // namespace ply
