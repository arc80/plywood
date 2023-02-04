/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-codec/Muxer.h>

using namespace ply;

PLY_INLINE s16 floatToS16(float v) {
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
        PLY_ASSERT(buf.format.numChannels == 1);
        PLY_ASSERT(buf.format.sampleType == audio::Format::SampleType::S16);
        PLY_ASSERT(buf.format.stride == 2);
        PLY_ASSERT(buf.sampleRate == 44100.f);

        s16* cur = (s16*) buf.samples;
        s16* end = (s16*) buf.getEndPtr();
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
            *cur += floatToS16(value * this->amplitude);
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
        PLY_ASSERT(buf.format.numChannels == 1);
        PLY_ASSERT(buf.format.sampleType == audio::Format::SampleType::S16);
        PLY_ASSERT(buf.format.stride == 2);
        PLY_ASSERT(buf.sampleRate == 44100.f);

        s16* cur = (s16*) buf.samples;
        s16* end = (s16*) buf.getEndPtr();
        for (; cur < end; cur++) {
            if (this->duration <= 0)
                return false;
            float value = 0.f;
            float raw = this->random.nextFloat() * 2.f - 1.f;
            this->w = mix(this->w, raw, 0.8f);
            value = this->w;
            *cur += floatToS16(value * this->amplitude);
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
        PLY_ASSERT(buf.format.numChannels == 1);
        PLY_ASSERT(buf.format.sampleType == audio::Format::SampleType::S16);
        PLY_ASSERT(buf.format.stride == 2);
        PLY_ASSERT(buf.sampleRate == 44100.f);

        s16* cur = (s16*) buf.samples;
        s16* end = (s16*) buf.getEndPtr();
        for (; cur < end; cur++) {
            if (this->pos >= 1.f)
                return false;
            float value = 0.5f - cosf(this->pos * 2 * Pi) * 0.5f;
            value *= (sinf(this->pos * 5 * Pi) + 0.2f * (this->random.nextFloat() * 2.f - 1.f));
            *cur += floatToS16(value * this->amplitude);
            this->pos += this->step;
        }
        return true;
    }
};

struct Event {
    float qTime;
    VoiceType type;
    float note;
    float qDur;
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
    Owned<OutPipe> out = FileSystem.openPipeForWrite("music-sample.mp3");
    AudioOptions audioOpts;
    Owned<Muxer> muxer = createMuxer(out, nullptr, &audioOpts, "mp3");

    sort(events.view(), [](const Event& a, const Event& b) { return a.qTime < b.qTime; });

    float secsPerBeat = 60.f / 100.f;
    s32 sampleTime = s32(44100.f * events[0].qTime * (secsPerBeat * 0.25f));
    Array<Owned<Voice>> voices;
    u32 eventIdx = 0;
    while (eventIdx < events.numItems()) {
        audio::Buffer buffer;
        muxer->beginAudioFrame(buffer);
        memset(buffer.samples, 0, buffer.getSizeBytes());
        u32 sampleOfs = 0;
        while (sampleOfs < buffer.numSamples && eventIdx < events.numItems()) {
            const Event* nextEvt = events.get(eventIdx);
            s32 nextEventTime = s32(44100.f * nextEvt->qTime * (secsPerBeat * 0.25f));
            s32 samplesToFill = min((s32) (buffer.numSamples - sampleOfs), (nextEventTime - sampleTime));
            if (samplesToFill > 0) {
                for (u32 i = 0; i < voices.numItems();) {
                    if (voices[i]->play(buffer.getSubregion(sampleOfs, samplesToFill))) {
                        i++;
                    } else {
                        voices.eraseQuick(i);
                    }
                }
                sampleTime += samplesToFill;
                sampleOfs += samplesToFill;
            } else {
                if (nextEvt->type == VoiceType::Kick) {
                    voices.append(new Voice_Kick{nextEvt->qDur * (secsPerBeat * 0.25f),
                                                 nextEvt->amplitude * 0.9f});
                } else if (nextEvt->type == VoiceType::Noise) {
                    voices.append(new Voice_Noise{nextEvt->qDur * (secsPerBeat * 0.25f),
                                                  nextEvt->amplitude * 0.9f});
                } else {
                    voices.append(new Voice_Tone{nextEvt->type, nextEvt->note,
                                                 nextEvt->qDur * (secsPerBeat * 0.25f),
                                                 nextEvt->amplitude * 0.9f});
                }
                eventIdx++;
            }
        }
        muxer->endAudioFrame(sampleOfs);
    }
    return 0;
}
