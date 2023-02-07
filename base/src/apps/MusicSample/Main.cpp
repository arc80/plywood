/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-codec/Muxer.h>

using namespace ply;

inline s16 float_to_s16(float v) {
    if (v >= 1.f)
        return 32767;
    else if (v < -1.f)
        return -32768;
    else
        return s16(32767.f * v);
}

enum class VoiceType {
    Square,
    Triangle,
    Noise,
    Kick,
};

struct Voice {
    virtual ~Voice() {
    }
    virtual bool play(const audio::Buffer& buf) = 0;
};

struct Voice_Tone : Voice {
    VoiceType type = VoiceType::Square;
    float pos = 0;
    float step = 0;
    float amplitude = 0.1f;
    s32 duration = 0;

    Voice_Tone(VoiceType type, float note, float duration, float amplitude) {
        this->type = type;
        this->step = 440.f * powf(2.f, (note - 9.f) / 12.f) / 44100.f;
        this->duration = s32(duration * 44100.f);
        this->amplitude = amplitude;
    }

    virtual bool play(const audio::Buffer& buf) override {
        PLY_ASSERT(buf.format.num_channels == 1);
        PLY_ASSERT(buf.format.sample_type == audio::Format::SampleType::S16);
        PLY_ASSERT(buf.format.stride == 2);
        PLY_ASSERT(buf.sample_rate == 44100.f);

        s16* cur = (s16*) buf.samples;
        s16* end = (s16*) buf.get_end_ptr();
        for (; cur < end; cur++) {
            if (this->duration <= 0)
                return false;
            float value = 0.f;
            if (this->type == VoiceType::Square) {
                value = (this->pos < 0.5f ? 1.f : -1.f);
            } else {
                float x = fmodf(this->pos + 0.25f, 1.f);
                value = (x < 0.5f ? x * 4.f : (1.f - x) * 4.f) - 1.f;
            }
            *cur += float_to_s16(value * this->amplitude);
            this->pos = fmodf(this->pos + this->step, 1.f);
            this->duration--;
        }
        return true;
    }
};

struct Voice_Noise : Voice {
    float amplitude = 0.1f;
    s32 duration = 0;
    float w = 0;
    Random random;

    Voice_Noise(float duration, float amplitude) : random{0} {
        this->duration = s32(duration * 44100.f);
        this->amplitude = amplitude;
    }

    virtual bool play(const audio::Buffer& buf) override {
        PLY_ASSERT(buf.format.num_channels == 1);
        PLY_ASSERT(buf.format.sample_type == audio::Format::SampleType::S16);
        PLY_ASSERT(buf.format.stride == 2);
        PLY_ASSERT(buf.sample_rate == 44100.f);

        s16* cur = (s16*) buf.samples;
        s16* end = (s16*) buf.get_end_ptr();
        for (; cur < end; cur++) {
            if (this->duration <= 0)
                return false;
            float value = 0.f;
            float raw = this->random.next_float() * 2.f - 1.f;
            this->w = mix(this->w, raw, 0.8f);
            value = this->w;
            *cur += float_to_s16(value * this->amplitude);
            this->duration--;
        }
        return true;
    }
};

struct Voice_Kick : Voice {
    float pos = 0;
    float step = 0;
    float amplitude = 0.1f;
    Random random;

    Voice_Kick(float duration, float amplitude) : random{0} {
        this->step = 1.f / (duration * 44100.f);
        this->amplitude = amplitude;
    }

    virtual bool play(const audio::Buffer& buf) override {
        PLY_ASSERT(buf.format.num_channels == 1);
        PLY_ASSERT(buf.format.sample_type == audio::Format::SampleType::S16);
        PLY_ASSERT(buf.format.stride == 2);
        PLY_ASSERT(buf.sample_rate == 44100.f);

        s16* cur = (s16*) buf.samples;
        s16* end = (s16*) buf.get_end_ptr();
        for (; cur < end; cur++) {
            if (this->pos >= 1.f)
                return false;
            float value = 0.5f - cosf(this->pos * 2 * Pi) * 0.5f;
            value *= (sinf(this->pos * 5 * Pi) +
                      0.2f * (this->random.next_float() * 2.f - 1.f));
            *cur += float_to_s16(value * this->amplitude);
            this->pos += this->step;
        }
        return true;
    }
};

struct Event {
    float q_time;
    VoiceType type;
    float note;
    float q_dur;
    float amplitude;
};

static constexpr float kofs = -0.17f;
static constexpr float rofs = -0.1f;

Array<Event> events = {
    // clang-format off

    // Melody
    {0,         VoiceType::Square,     16,  0.8f, 0.05f},
    {0,         VoiceType::Square,     16,  0.8f, 0.05f},
    {0,         VoiceType::Square,     6,   0.8f, 0.05f},
    {1,         VoiceType::Square,     16,  0.8f, 0.05f},
    {1,         VoiceType::Square,     6,   0.8f, 0.05f},
    {3,         VoiceType::Square,     16,  0.8f, 0.05f},
    {3,         VoiceType::Square,     6,   0.8f, 0.05f},
    {5,         VoiceType::Square,     12,  0.8f, 0.05f},
    {5,         VoiceType::Square,     6,   0.8f, 0.05f},
    {6,         VoiceType::Square,     16,  0.8f, 0.05f},
    {6,         VoiceType::Square,     6,   0.8f, 0.05f},
    {8,         VoiceType::Square,     19,  0.8f, 0.05f},
    {8,         VoiceType::Square,     11,  0.8f, 0.05f},
    {12,        VoiceType::Square,     7,   0.8f, 0.05f},

    // Bass
    {0,         VoiceType::Triangle,   -10, 0.8f, 0.22f},
    {1,         VoiceType::Triangle,   -10, 0.8f, 0.22f},
    {3,         VoiceType::Triangle,   -10, 0.8f, 0.22f},
    {5,         VoiceType::Triangle,   -10, 0.8f, 0.22f},
    {6,         VoiceType::Triangle,   -10, 0.8f, 0.22f},
    {8,         VoiceType::Triangle,   7,   0.8f, 0.22f},
    {12,        VoiceType::Triangle,   -5,  0.8f, 0.22f},

    // Kick
    {0 + kofs,  VoiceType::Kick,       0,   0.25f,  0.1f},
    {1 + kofs,  VoiceType::Kick,       0,   0.25f,  0.1f},
    {2 + kofs,  VoiceType::Kick,       0,   0.25f,  0.1f},
    {3 + kofs,  VoiceType::Kick,       0,   0.25f,  0.1f},
    {5 + kofs,  VoiceType::Kick,       0,   0.25f,  0.1f},
    {6 + kofs,  VoiceType::Kick,       0,   0.25f,  0.1f},
    {8 + kofs,  VoiceType::Kick,       0,   0.25f,  0.1f},
    {11 + kofs, VoiceType::Kick,       0,   0.25f,  0.1f},
    {13 + kofs, VoiceType::Kick,       0,   0.25f,  0.1f},
    {14 + kofs, VoiceType::Kick,       0,   0.25f,  0.1f},
    {15 + kofs, VoiceType::Kick,       0,   0.25f,  0.1f},

    // Rhythm
    {0 + rofs,  VoiceType::Noise,      0,   0.5f,  0.11f},
    {2 + rofs,  VoiceType::Noise,      0,   0.12f, 0.11f},
    {3 + rofs,  VoiceType::Noise,      0,   0.5f,  0.11f},
    {5 + rofs,  VoiceType::Noise,      0,   0.12f, 0.11f},
    {6 + rofs,  VoiceType::Noise,      0,   0.5f,  0.11f},
    {8 + rofs,  VoiceType::Noise,      0,   0.5f,  0.11f},
    {11 + rofs, VoiceType::Noise,      0,   0.5f,  0.11f},
    {13 + rofs, VoiceType::Noise,      0,   0.12f, 0.11f},
    {14 + rofs, VoiceType::Noise,      0,   0.12f, 0.11f},
    {15 + rofs, VoiceType::Noise,      0,   0.12f, 0.11f},

    {16,        VoiceType::Square,     0,   0.f,   0.f},
    // clang-format on
};

int main() {
    Owned<OutPipe> out = FileSystem.open_pipe_for_write("music-sample.mp3");
    AudioOptions audio_opts;
    Owned<Muxer> muxer = create_muxer(out, nullptr, &audio_opts, "mp3");

    sort(events.view(),
         [](const Event& a, const Event& b) { return a.q_time < b.q_time; });

    float secs_per_beat = 60.f / 100.f;
    s32 sample_time = s32(44100.f * events[0].q_time * (secs_per_beat * 0.25f));
    Array<Owned<Voice>> voices;
    u32 event_idx = 0;
    while (event_idx < events.num_items()) {
        audio::Buffer buffer;
        muxer->begin_audio_frame(buffer);
        memset(buffer.samples, 0, buffer.get_size_bytes());
        u32 sample_ofs = 0;
        while (sample_ofs < buffer.num_samples && event_idx < events.num_items()) {
            const Event* next_evt = events.get(event_idx);
            s32 next_event_time =
                s32(44100.f * next_evt->q_time * (secs_per_beat * 0.25f));
            s32 samples_to_fill = min((s32) (buffer.num_samples - sample_ofs),
                                      (next_event_time - sample_time));
            if (samples_to_fill > 0) {
                for (u32 i = 0; i < voices.num_items();) {
                    if (voices[i]->play(
                            buffer.get_subregion(sample_ofs, samples_to_fill))) {
                        i++;
                    } else {
                        voices.erase_quick(i);
                    }
                }
                sample_time += samples_to_fill;
                sample_ofs += samples_to_fill;
            } else {
                if (next_evt->type == VoiceType::Kick) {
                    voices.append(
                        new Voice_Kick{next_evt->q_dur * (secs_per_beat * 0.25f),
                                       next_evt->amplitude * 0.9f});
                } else if (next_evt->type == VoiceType::Noise) {
                    voices.append(
                        new Voice_Noise{next_evt->q_dur * (secs_per_beat * 0.25f),
                                        next_evt->amplitude * 0.9f});
                } else {
                    voices.append(
                        new Voice_Tone{next_evt->type, next_evt->note,
                                       next_evt->q_dur * (secs_per_beat * 0.25f),
                                       next_evt->amplitude * 0.9f});
                }
                event_idx++;
            }
        }
        muxer->end_audio_frame(sample_ofs);
    }
    return 0;
}
