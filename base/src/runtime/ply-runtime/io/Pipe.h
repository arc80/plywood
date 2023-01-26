/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                   ┃
┃    ╱   ╱╲    Plywood Multimedia Toolkit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/          ┃
┃    └──┴┴┴┘                                 ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/

#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/io/OutStream.h>
#include <ply-runtime/string/StringView.h>
#include <ply-runtime/string/Label.h>

namespace ply {

enum SeekDirection {
    Seek_Absolute,
    Seek_Relative,
    Seek_End,
};

//  ▄▄▄▄        ▄▄▄▄▄  ▄▄
//   ██  ▄▄▄▄▄  ██  ██ ▄▄ ▄▄▄▄▄   ▄▄▄▄
//   ██  ██  ██ ██▀▀▀  ██ ██  ██ ██▄▄██
//  ▄██▄ ██  ██ ██     ██ ██▄▄█▀ ▀█▄▄▄
//                        ██

struct InPipe {
    StringView type;
    template <typename T>
    T* cast() {
        PLY_ASSERT(this->type == T::Type);
        return static_cast<T*>(this);
    }

    InPipe(StringView type = {}) : type{type} {
    }
    virtual ~InPipe() = default;
    // read() only returns 0 at EOF. Otherwise, it blocks.
    virtual u32 read(MutStringView buf) = 0;
    virtual u64 get_file_size();
    virtual void seek(s64 offset, SeekDirection dir);
};

bool fill_buffer(MutStringView to_buf, InPipe* from_pipe);

#if PLY_TARGET_WIN32

// ┏━━━━━━━━━━━━━━━━━┓
// ┃  InPipe_Handle  ┃
// ┗━━━━━━━━━━━━━━━━━┛
struct InPipe_Handle : InPipe {
    static constexpr char* Type = "Handle";
    HANDLE handle = INVALID_HANDLE_VALUE;

    InPipe_Handle(HANDLE h) : InPipe{Type}, handle(h) {
    }
    virtual ~InPipe_Handle();
    virtual u32 read(MutStringView buf) override;
    virtual u64 get_file_size() override;
    virtual void seek(s64 offset, SeekDirection dir) override;
};

#endif // PLY_TARGET_WIN32

//   ▄▄▄▄          ▄▄   ▄▄▄▄▄  ▄▄
//  ██  ██ ▄▄  ▄▄ ▄██▄▄ ██  ██ ▄▄ ▄▄▄▄▄   ▄▄▄▄
//  ██  ██ ██  ██  ██   ██▀▀▀  ██ ██  ██ ██▄▄██
//  ▀█▄▄█▀ ▀█▄▄██  ▀█▄▄ ██     ██ ██▄▄█▀ ▀█▄▄▄
//                                ██

struct OutPipe {
    StringView type;
    // child_stream is unused by OutPipe_Handle/FD/Winsock.
    OutStream child_stream;

    OutPipe(StringView type = {}) : type{type} {
    }
    virtual ~OutPipe() = default;
    template <typename T>
    T* cast() {
        PLY_ASSERT(this->type == T::Type);
        return static_cast<T*>(this);
    }
    OutPipe* get_tail_pipe();

    // write() may block if connected to a blocked reader.
    virtual bool write(StringView buf) = 0;
    virtual void flush(bool hard = false);
    virtual void seek(s64 offset, SeekDirection dir);
};

#if PLY_TARGET_WIN32

// ┏━━━━━━━━━━━━━━━━━━┓
// ┃  OutPipe_Handle  ┃
// ┗━━━━━━━━━━━━━━━━━━┛
struct OutPipe_Handle : OutPipe {
    static constexpr char* Type = "Handle";
    HANDLE handle = INVALID_HANDLE_VALUE;

    OutPipe_Handle(HANDLE h) : OutPipe{Type}, handle(h) {
    }
    virtual ~OutPipe_Handle();
    virtual bool write(StringView buf) override;
    virtual void flush(bool hard) override;
    virtual void seek(s64 offset, SeekDirection dir) override;
};

#endif // PLY_TARGET_WIN32

} // namespace ply
