/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                   ┃
┃    ╱   ╱╲    Plywood Multimedia Toolkit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/          ┃
┃    └──┴┴┴┘                                 ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/

#pragma once
#include <ply-runtime.h>
#include <ply-runtime/container/BlockList.h>
#include <ply-runtime/container/Owned.h>

namespace ply {

//   ▄▄▄▄          ▄▄    ▄▄▄▄   ▄▄
//  ██  ██ ▄▄  ▄▄ ▄██▄▄ ██  ▀▀ ▄██▄▄ ▄▄▄▄▄   ▄▄▄▄   ▄▄▄▄  ▄▄▄▄▄▄▄
//  ██  ██ ██  ██  ██    ▀▀▀█▄  ██   ██  ▀▀ ██▄▄██  ▄▄▄██ ██ ██ ██
//  ▀█▄▄█▀ ▀█▄▄██  ▀█▄▄ ▀█▄▄█▀  ▀█▄▄ ██     ▀█▄▄▄  ▀█▄▄██ ██ ██ ██
//

struct OutStream {
    struct Status {
        u32 block_size : 29;
        u32 is_pipe_owner : 1;
        u32 auto_shim : 1;
        u32 eof : 1;
        Status() : block_size{4096}, is_pipe_owner{0}, auto_shim{0}, eof{0} {
        }
    };

    char* start_byte = nullptr;
    char* cur_byte = nullptr;
    char* end_byte = nullptr;
    Reference<BlockList::Footer> block;
    Reference<BlockList::Footer> head_block; // nullptr if writing to out_pipe
    OutPipe* out_pipe = nullptr;             // nullptr if writing to memory
    Status status;

    // ┏━━━━━━━━━━━━━━━━┓
    // ┃  Constructors  ┃
    // ┗━━━━━━━━━━━━━━━━┛
    OutStream() = default;
    // out_pipe can be nullptr, in which case is_open() will return false.
    OutStream(OutPipe* out_pipe, bool is_pipe_owner);
    OutStream(Owned<OutPipe>&& out_pipe) : OutStream{out_pipe.release(), true} {
    }
    OutStream(OutStream&& other);
    ~OutStream();

    void operator=(OutStream&& other) {
        PLY_ASSERT(this != &other);
        this->~OutStream();
        new (this) OutStream{std::move(other)};
    }

    // ┏━━━━━━━━━━━┓
    // ┃  Methods  ┃
    // ┗━━━━━━━━━━━┛
    u64 get_seek_pos() const;
    void flush(bool hard = false);

    bool is_open() const {
        return this->cur_byte != nullptr;
    }
    void close() {
        *this = {};
    }
    explicit operator bool() const {
        return this->cur_byte != nullptr;
    }
    bool at_eof() const {
        return this->status.eof != 0;
    }
    u32 num_writable_bytes() const {
        return safeDemote<u32>(this->end_byte - this->cur_byte);
    }
    MutStringView view_writable() {
        return {cur_byte, safeDemote<u32>(end_byte - cur_byte)};
    }
    bool make_writable();
    bool ensure_writable() {
        if (this->cur_byte < this->end_byte)
            return true;
        return this->make_writable();
    }
    OutStream& operator<<(char c) {
        if (this->cur_byte >= this->end_byte) {
            if (!this->make_writable())
                return *this;
        }
        *this->cur_byte++ = c;
        return *this;
    }
    OutStream& operator<<(StringView str);
    template <typename T, std::enable_if_t<std::is_arithmetic<T>::value, int> = 0>
    OutStream& operator<<(T c) {
        FormatArg{c}.print(*this);
        return *this;
    }
    template <typename T>
    void raw_write(const T& value) {
        *this << StringView{(const char*) &value, sizeof(T)};
    }

    void format(const FormatArg& arg) {
        arg.print_func(*this, arg);
    }
    void format_args(StringView fmt, ArrayView<const FormatArg> args);
    template <typename... Args>
    PLY_NO_INLINE void format(StringView fmt, const Args&... args) {
        FixedArray<FormatArg, sizeof...(Args)> fa{args...};
        this->format_args(fmt, fa);
    }
    String moveToString(); // Must be a MemOutStream.
};

struct MemOutStream : OutStream {
    MemOutStream();

    BlockList::Ref get_head_ref() {
        PLY_ASSERT(this->head_block);
        return {this->head_block, this->head_block->start()};
    }
};

struct ViewOutStream : OutStream {
    ViewOutStream(MutStringView view);
};

//------------------------------------------------------------------
// NativeEndianWriter
//------------------------------------------------------------------
class NativeEndianWriter {
public:
    OutStream& out;

    PLY_INLINE NativeEndianWriter(OutStream& out) : out(out) {
    }

    template <typename T>
    PLY_INLINE void write(const T& value) {
        out.raw_write(value);
    }
};

// ┏━━━━━━━━━━┓
// ┃  String  ┃
// ┗━━━━━━━━━━┛
template <typename... Args>
PLY_INLINE String String::format(StringView fmt, const Args&... args) {
    MemOutStream mout;
    mout.format(fmt, args...);
    return mout.moveToString();
}

template <typename T>
String to_string(T&& value) {
    MemOutStream mout;
    FormatArg{std::forward<T>(value)}.print(mout);
    return mout.moveToString();
}

} // namespace ply

#include <ply-runtime/io/impl/FormatString.h>
