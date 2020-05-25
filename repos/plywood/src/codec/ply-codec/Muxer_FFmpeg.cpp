/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/

// Heavily modified from ffmpeg/doc/examples/muxing.c
// Possible improvements:
// - It's odd that the first pts of the audio stream is -1024
// - Avoid sws/srs conversion steps when unnecessary
//      (but they will always be necessary when passing RGB frames to a YUV encoder)

#include <ply-codec/Muxer.h>

extern "C" {
#include <libavutil/avassert.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

namespace ply {

#define SCALE_FLAGS SWS_BICUBIC // FIXME: Should be point?

void log(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

struct TempString {
    char buf[AV_ERROR_MAX_STRING_SIZE] = {0};
    operator char*() {
        return buf;
    }
};

#undef av_err2str
#define av_err2str(errnum) av_make_error_string(TempString{}, AV_ERROR_MAX_STRING_SIZE, errnum)
#undef av_ts2str
#define av_ts2str(ts) av_ts_make_string(TempString{}, ts)
#undef av_ts2timestr
#define av_ts2timestr(ts, tb) av_ts_make_time_string(TempString{}, ts, tb)

static void log_packet(const AVFormatContext* fmt_ctx, const AVPacket* pkt) {
    AVRational* time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;
    log("pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
        av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base), av_ts2str(pkt->dts),
        av_ts2timestr(pkt->dts, time_base), av_ts2str(pkt->duration),
        av_ts2timestr(pkt->duration, time_base), pkt->stream_index);
}

class VideoEncoder {
public:
    AVFormatContext* oc = nullptr; // Container
    AVCodec* codec = nullptr;      // Was registered at startup
    AVStream* st = nullptr;        // Is owned by the external AVFormatContext
    AVCodecContext* enc = nullptr;
    AVFrame* frame = nullptr;
    AVFrame* tmp_frame = nullptr;
    struct SwsContext* sws_ctx;
    int64_t next_pts = 0;

    bool isValid() const {
        return enc != nullptr;
    }

    static AVFrame* alloc_picture(enum AVPixelFormat pix_fmt, int width, int height) {
        AVFrame* picture = av_frame_alloc();
        if (!picture) {
            log("Could not allocate frame\n");
            return nullptr;
        }

        picture->format = pix_fmt;
        picture->width = width;
        picture->height = height;

        // Allocate the buffers for the frame data.
        int ret = av_frame_get_buffer(picture, 32);
        if (ret < 0) {
            log("Could not allocate frame data.\n");
            av_frame_free(&picture);
            return nullptr;
        }
        return picture;
    }

    VideoEncoder(AVFormatContext* oc, AVDictionary* opt_arg, const VideoOptions* videoOpts)
        : oc(oc) {
        // Find AVCodec
        enum AVCodecID codec_id = oc->oformat->video_codec;
        codec = avcodec_find_encoder(codec_id);
        if (!codec) {
            log("Could not find encoder for '%s'\n", avcodec_get_name(codec_id));
            cleanup();
            return;
        }
        PLY_ASSERT(codec->type == AVMEDIA_TYPE_VIDEO);

        // Create AVStream
        st = avformat_new_stream(oc, NULL);
        if (!st) {
            log("Could not allocate stream\n");
            cleanup();
            return;
        }
        st->id = oc->nb_streams - 1;

        // Create AVCodecContext
        enc = avcodec_alloc_context3(codec);
        if (!enc) {
            log("Could not alloc an encoding context\n");
            cleanup();
            return;
        }

        // Configure AVCodecContext & AVStream
        enc->codec_id = codec_id;
        enc->bit_rate = videoOpts->bitRate;
        // Resolution must be a multiple of two
        enc->width = videoOpts->width;
        enc->height = videoOpts->height;
        st->time_base = AVRational{1, 30};
        enc->time_base = st->time_base;
        enc->gop_size = 12; // emit one intra frame every twelve frames at most
        enc->pix_fmt = AV_PIX_FMT_YUV420P;
        if (enc->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
            // Needed to avoid using macroblocks in which some coeffs overflow.
            // This does not happen with normal video, it just happens here as
            // the motion of the chroma plane does not match the luma plane.
            enc->mb_decision = 2;
        }

        // Some formats want stream headers to be separate
        if (oc->oformat->flags & AVFMT_GLOBALHEADER)
            enc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

        // Open the AVCodec
        AVDictionary* opt = NULL;
        av_dict_copy(&opt, opt_arg, 0);
        int ret = avcodec_open2(enc, codec, &opt);
        av_dict_free(&opt);
        if (ret < 0) {
            log("Could not open video codec: %s\n", av_err2str(ret));
            cleanup();
            return;
        }

        // Copy the stream parameters to the muxer
        ret = avcodec_parameters_from_context(st->codecpar, enc);
        if (ret < 0) {
            log("Could not copy the stream parameters\n");
            cleanup();
            return;
        }

        // Allocate and init a re-usable frame
        frame = alloc_picture(enc->pix_fmt, enc->width, enc->height);
        if (!frame) {
            cleanup();
            return;
        }

        // Allocate and init a temporary RGB frame
        tmp_frame = alloc_picture(AV_PIX_FMT_BGRA, enc->width, enc->height);
        if (!tmp_frame) {
            cleanup();
            return;
        }

        // Create SwsContext (software scaler)
        sws_ctx = sws_getContext(enc->width, enc->height, AV_PIX_FMT_BGRA, enc->width, enc->height,
                                 enc->pix_fmt, SCALE_FLAGS, NULL, NULL, NULL);
        if (!sws_ctx) {
            log("Could not initialize the conversion context\n");
            cleanup();
            return;
        }
    }

    void cleanup() {
        sws_freeContext(sws_ctx);
        sws_ctx = nullptr;
        av_frame_free(&tmp_frame);
        av_frame_free(&frame);
        avcodec_free_context(&enc);
        st = nullptr;
        codec = nullptr;
        oc = nullptr;
    }

    ~VideoEncoder() {
    }

    void beginFrame(image::Image& rgbIm) {
        if (!enc) {
            // Return empty buffer
            rgbIm = image::Image{nullptr, 0, 0, 0, image::Format::BGRA};
            return;
        }

        rgbIm = image::Image{tmp_frame->data[0], tmp_frame->linesize[0], enc->width, enc->height,
                             image::Format::BGRA};
    }

    void endFrame() {
        if (!enc)
            return;

        // Convert tmp_frame to frame
        sws_scale(sws_ctx, (const uint8_t* const*) tmp_frame->data, tmp_frame->linesize, 0,
                  enc->height, frame->data, frame->linesize);

        // Encode
        frame->pts = next_pts++;
        int ret = avcodec_send_frame(enc, frame);
        if (ret < 0) {
            log("Error encoding video frame: %s\n", av_err2str(ret));
            cleanup();
            return;
        }

        processVideo();
    }

    void processVideo() {
        AVPacket pkt = {0};
        av_init_packet(&pkt);

        for (;;) {
            int ret = avcodec_receive_packet(enc, &pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                return;
            if (ret < 0) {
                log("Error while writing video frame: %s\n", av_err2str(ret));
                cleanup();
                return;
            }

            // Rescale output packet timestamp values from codec to stream timebase
            av_packet_rescale_ts(&pkt, enc->time_base, st->time_base);
            pkt.stream_index = st->index;

            // Write the compressed frame to the media file
            log_packet(oc, &pkt);
            ret = av_interleaved_write_frame(oc, &pkt);
            if (ret < 0) {
                log("Error writing compressed video to media file: %s\n", av_err2str(ret));
                cleanup();
                return;
            }
        }
    }

    void flushVideo() {
        if (!enc)
            return;

        int ret = avcodec_send_frame(enc, NULL);
        if (ret < 0) {
            log("Error terminating video stream: %s\n", av_err2str(ret));
            cleanup();
            return;
        }
        processVideo();
    }
};

class AudioEncoder {
public:
    AVFormatContext* oc = nullptr; // Container
    AVCodec* codec = nullptr;      // Was registered at startup
    AVStream* st = nullptr;        // Is owned by the external AVFormatContext
    AVCodecContext* enc = nullptr;
    AVFrame* frame = nullptr;
    AVFrame* tmp_frame = nullptr;
    struct SwrContext* swr_ctx = nullptr;
    int64_t next_pts = 0;
    int samples_count = 0;

    bool isValid() const {
        return enc != nullptr;
    }

    static AVFrame* alloc_audio_frame(enum AVSampleFormat sample_fmt, uint64_t channel_layout,
                                      int sample_rate, int nb_samples) {
        AVFrame* frame = av_frame_alloc();
        int ret;

        if (!frame) {
            log("Error allocating an audio frame\n");
            return nullptr;
        }

        frame->format = sample_fmt;
        frame->channel_layout = channel_layout;
        frame->sample_rate = sample_rate;
        frame->nb_samples = nb_samples;

        if (nb_samples) {
            ret = av_frame_get_buffer(frame, 0);
            if (ret < 0) {
                log("Error allocating an audio buffer\n");
                return nullptr;
            }
        }
        return frame;
    }

    AudioEncoder(AVFormatContext* oc, AVDictionary* opt_arg, const AudioOptions* audioOpts)
        : oc(oc) {
        // Find AVCodec
        enum AVCodecID codec_id = oc->oformat->audio_codec;
        codec = avcodec_find_encoder(codec_id);
        if (!codec) {
            log("Could not find encoder for '%s'\n", avcodec_get_name(codec_id));
            cleanup();
            return;
        }
        PLY_ASSERT(codec->type == AVMEDIA_TYPE_AUDIO);

        // Create AVStream
        st = avformat_new_stream(oc, NULL);
        if (!st) {
            log("Could not allocate stream\n");
            cleanup();
            return;
        }
        st->id = oc->nb_streams - 1;

        // Create AVCodecContext
        enc = avcodec_alloc_context3(codec);
        if (!enc) {
            log("Could not alloc an encoding context\n");
            cleanup();
            return;
        }

        // Configure AVCodecContext & AVStream
        enc->sample_fmt = codec->sample_fmts ? codec->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
        enc->bit_rate = audioOpts->bitRate;
        enc->sample_rate = audioOpts->sampleRate;
        if (codec->supported_samplerates) {
            enc->sample_rate = codec->supported_samplerates[0];
            for (int i = 0; codec->supported_samplerates[i]; i++) {
                if (codec->supported_samplerates[i] == (int) audioOpts->sampleRate)
                    enc->sample_rate = audioOpts->sampleRate;
            }
        }
        enc->channels = av_get_channel_layout_nb_channels(enc->channel_layout);
        uint64_t desiredChannelLayout = AV_CH_LAYOUT_MONO;
        if (audioOpts->numChannels == 1) {
            desiredChannelLayout = AV_CH_LAYOUT_MONO;
        } else if (audioOpts->numChannels == 2) {
            desiredChannelLayout = AV_CH_LAYOUT_STEREO;
        } else {
            PLY_ASSERT(0); // Unsupported
        }
        enc->channel_layout = desiredChannelLayout;
        if (codec->channel_layouts) {
            enc->channel_layout = codec->channel_layouts[0];
            for (int i = 0; codec->channel_layouts[i]; i++) {
                if (codec->channel_layouts[i] == desiredChannelLayout)
                    enc->channel_layout = desiredChannelLayout;
            }
        }
        enc->channels = av_get_channel_layout_nb_channels(enc->channel_layout);
        st->time_base = AVRational{1, enc->sample_rate};

        // Some formats want stream headers to be separate
        if (oc->oformat->flags & AVFMT_GLOBALHEADER)
            enc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

        // Open the AVCodec
        AVDictionary* opt = NULL;
        av_dict_copy(&opt, opt_arg, 0);
        int ret = avcodec_open2(enc, codec, &opt);
        av_dict_free(&opt);
        if (ret < 0) {
            log("Could not open audio codec: %s\n", av_err2str(ret));
            cleanup();
            return;
        }

        // Copy the stream parameters to the muxer
        ret = avcodec_parameters_from_context(st->codecpar, enc);
        if (ret < 0) {
            log("Could not copy the stream parameters\n");
            cleanup();
            return;
        }

        // Create AVFrames
        int nb_samples =
            (enc->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE) ? 10000 : enc->frame_size;
        frame =
            alloc_audio_frame(enc->sample_fmt, enc->channel_layout, enc->sample_rate, nb_samples);
        if (!frame) {
            cleanup();
            return;
        }
        tmp_frame =
            alloc_audio_frame(AV_SAMPLE_FMT_S16, enc->channel_layout, enc->sample_rate, nb_samples);
        if (!tmp_frame) {
            cleanup();
            return;
        }

        // Create SwrContext (resampler)
        swr_ctx = swr_alloc();
        if (!swr_ctx) {
            log("Could not allocate resampler context\n");
            cleanup();
            return;
        }

        // Set SwrContext options
        av_opt_set_int(swr_ctx, "in_channel_count", enc->channels, 0);
        av_opt_set_int(swr_ctx, "in_sample_rate", enc->sample_rate, 0);
        av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", AV_SAMPLE_FMT_S16, 0);
        av_opt_set_int(swr_ctx, "out_channel_count", enc->channels, 0);
        av_opt_set_int(swr_ctx, "out_sample_rate", enc->sample_rate, 0);
        av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", enc->sample_fmt, 0);

        // Initialize SwrContext
        if ((ret = swr_init(swr_ctx)) < 0) {
            log("Failed to initialize the resampling context\n");
            cleanup();
            return;
        }
    }

    void cleanup() {
        swr_free(&swr_ctx);
        av_frame_free(&tmp_frame);
        av_frame_free(&frame);
        avcodec_free_context(&enc);
        st = nullptr;
        codec = nullptr;
        oc = nullptr;
    }

    ~AudioEncoder() {
        cleanup();
    }

    void beginFrame(audio::Buffer& buffer) {
        if (!enc) {
            // Return empty buffer
            buffer = audio::Buffer{nullptr, 0, 44100.f, audio::Format::StereoS16};
            return;
        }

        tmp_frame->pts = next_pts;
        next_pts += tmp_frame->nb_samples;
        buffer = audio::Buffer{
            tmp_frame->data[0], (u32) tmp_frame->nb_samples, (float) enc->sample_rate,
            audio::Format::encode((u8) enc->channels, audio::Format::SampleType::S16)};
    }

    void endFrame(u32 numSamples) {
        if (!enc)
            return;

        // Compute destination number of samples
        int dst_nb_samples =
            (int) av_rescale_rnd(swr_get_delay(swr_ctx, enc->sample_rate) + tmp_frame->nb_samples,
                                 enc->sample_rate, enc->sample_rate, AV_ROUND_UP);
        PLY_ASSERT(dst_nb_samples == frame->nb_samples);

        // When we pass a frame to the encoder, it may keep a reference to it internally.
        // Make sure we do not overwrite it here.
        int ret = av_frame_make_writable(frame);
        if (ret < 0) {
            cleanup();
            return;
        }

        // Note: Only the last frame is allowed to have numSamples < tmp_frame->nb_samples.
        PLY_ASSERT(numSamples <= safeDemote<u32>(tmp_frame->nb_samples));

        // Convert samples from native format to destination codec format, using the resampler
        ret = swr_convert(swr_ctx, frame->data, dst_nb_samples, (const uint8_t**) tmp_frame->data,
                          numSamples);
        if (ret < 0) {
            log("Error while converting\n");
            cleanup();
            return;
        }

        // Send converted samples to encoder
        frame->pts = av_rescale_q(samples_count, AVRational{1, enc->sample_rate}, enc->time_base);
        samples_count += dst_nb_samples;
        ret = avcodec_send_frame(enc, frame);
        if (ret < 0) {
            log("Error encoding audio frame: %s\n", av_err2str(ret));
            cleanup();
            return;
        }

        processAudio();
    }

    void processAudio() {
        AVPacket pkt = {0};
        av_init_packet(&pkt);

        for (;;) {
            int ret = avcodec_receive_packet(enc, &pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                return;
            if (ret < 0) {
                log("Error while writing audio frame: %s\n", av_err2str(ret));
                cleanup();
                return;
            }

            // Rescale output packet timestamp values from codec to stream timebase
            av_packet_rescale_ts(&pkt, enc->time_base, st->time_base);
            pkt.stream_index = st->index;

            // Write the compressed frame to the media file
            log_packet(oc, &pkt);
            ret = av_interleaved_write_frame(oc, &pkt);
            if (ret < 0) {
                log("Error writing compressed audio to media file: %s\n", av_err2str(ret));
                cleanup();
                return;
            }
        }
    }

    void flushAudio() {
        if (!enc)
            return;

        int ret = avcodec_send_frame(enc, NULL);
        if (ret < 0) {
            log("Error terminating audio stream: %s\n", av_err2str(ret));
            cleanup();
            return;
        }
        processAudio();
    }
};

class Muxer_FFmpeg : public Muxer {
public:
    AVDictionary* opt = NULL;
    AVFormatContext* oc = nullptr;
    VideoEncoder* videoEnc = nullptr;
    AudioEncoder* audioEnc = nullptr;
    AVIOContext* avioCtx = nullptr;
    bool m_isValid = false;

    Muxer_FFmpeg(OutPipe* out, const VideoOptions* videoOpts, const AudioOptions* audioOpts,
                 StringView containerFormat) {
        int ret;

        // for (i = 2; i+1 < argc; i+=2) {
        //    if (!strcmp(argv[i], "-flags") || !strcmp(argv[i], "-fflags"))
        //        av_dict_set(&opt, argv[i]+1, argv[i+1], 0);
        //}

        // Create AVFormatContext
        AVOutputFormat* oformat =
            av_guess_format(containerFormat.withNullTerminator().bytes, NULL, NULL);
        if (!oformat) {
            cleanup();
            return;
        }

        avformat_alloc_output_context2(&oc, oformat, NULL, NULL);
        if (!oc) {
            cleanup();
            return;
        }

        // Create video/audio encoders
        if (videoOpts) {
            videoEnc = new VideoEncoder(oc, opt, videoOpts);
        }
        if (audioOpts) {
            audioEnc = new AudioEncoder(oc, opt, audioOpts);
            sampleRate = (float) audioEnc->enc->sample_rate;
        }

        // Create AVIOContext, to supply our output callbacks
        PLY_ASSERT((oc->oformat->flags & AVFMT_NOFILE) == 0);
        int bufferSize = 4096;
        u8* buffer = (u8*) av_malloc(bufferSize);
        if (!buffer) {
            log("av_malloc failed\n");
            cleanup();
            return;
        }
        auto writePacket = [](void* opaque, uint8_t* buf, int len) -> int {
            return reinterpret_cast<OutPipe*>(opaque)->write({buf, safeDemote<u32>(len)});
        };
        auto seek = [](void* opaque, int64_t offset, int whence) -> int64_t {
            SeekDir seekDir = SeekDir::Cur;
            switch (whence) {
                case SEEK_SET: {
                    seekDir = SeekDir::Set;
                    break;
                }
                case SEEK_CUR: {
                    seekDir = SeekDir::Cur;
                    break;
                }
                case SEEK_END: {
                    seekDir = SeekDir::End;
                    break;
                }
                default: {
                    PLY_ASSERT(0); // Unrecognized whence
                }
            }
            return reinterpret_cast<OutPipe*>(opaque)->seek(offset, seekDir);
        };
        avioCtx = avio_alloc_context(buffer, bufferSize, 1, out, NULL, writePacket, seek);
        if (!avioCtx) {
            log("avio_alloc_context failed\n");
            av_free(buffer);
            cleanup();
            return;
        }
        oc->pb = avioCtx;

        // Log the format
        // FIXME: Should we pass logging callbacks into FFmpeg?
        // av_dump_format(oc, 0, nullTermFilename.bytes, 1);

        // Write the stream header, if any.
        ret = avformat_write_header(oc, &opt);
        if (ret < 0) {
            log("Error occurred when opening output file: %s\n", av_err2str(ret));
            cleanup();
            return;
        }

        av_dict_free(&opt);
        m_isValid = true;
    }

    void cleanup() {
        if (avioCtx) {
            av_free(avioCtx->buffer);
            av_free(avioCtx);
            avioCtx = nullptr;
        }
        delete audioEnc; // nullptr is OK
        audioEnc = nullptr;
        delete videoEnc; // nullptr is OK
        videoEnc = nullptr;
        if (oc) {
            avformat_free_context(oc);
            oc = nullptr;
        }
        av_dict_free(&opt);
    }

    virtual ~Muxer_FFmpeg() override {
        if (this->videoEnc) {
            this->videoEnc->flushVideo();
        }
        if (this->audioEnc) {
            this->audioEnc->flushAudio();
        }
        if (m_isValid) {
            // Write the trailer, if any. The trailer must be written before you
            // close the CodecContexts open when you wrote the header; otherwise
            // av_write_trailer() may try to use memory that was freed on
            // av_codec_close().
            av_write_trailer(oc);
        }
        cleanup();
    }

    virtual void beginVideoFrame(image::Image& rgbIm) override {
        videoEnc->beginFrame(rgbIm);
    }

    virtual void endVideoFrame() override {
        videoEnc->endFrame();
    }

    virtual double getVideoTime() override {
        return videoEnc->next_pts * av_q2d(videoEnc->enc->time_base);
    }

    virtual u32 getVideoFrameNumber() override {
        return (u32) videoEnc->next_pts;
    }

    virtual void flushVideo() override {
        videoEnc->flushVideo();
    }

    virtual void beginAudioFrame(audio::Buffer& buffer) override {
        audioEnc->beginFrame(buffer);
    }

    virtual void endAudioFrame(u32 numSamples) override {
        audioEnc->endFrame(numSamples);
    }

    virtual double getAudioTime() override {
        return audioEnc->next_pts * av_q2d(audioEnc->enc->time_base);
    }

    virtual void flushAudio() override {
        audioEnc->flushAudio();
    }
};

Owned<Muxer> createMuxer(OutPipe* out, const VideoOptions* videoOpts, const AudioOptions* audioOpts,
                         StringView containerFormat) {
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58, 9, 100)
    static Initializer init{[] {
        av_register_all();
        avcodec_register_all();
    }};
#endif
    return new Muxer_FFmpeg(out, videoOpts, audioOpts, containerFormat);
}

} // namespace ply
