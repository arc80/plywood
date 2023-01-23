/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                   ┃
┃    ╱   ╱╲    Plywood Multimedia Toolkit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/          ┃
┃    └──┴┴┴┘                                 ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/

#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/container/BlockList.h>
#include <ply-runtime/io/Pipe.h>
#include <ply-runtime/container/Owned.h>
#include <ply-runtime/string/String.h>
#include <ply-runtime/container/FixedArray.h>

namespace ply {

namespace fmt {
template <typename T>
struct TypePrinter;
} // namespace fmt

struct FormatArg {
    void (*func)(OutStream&, const void*) = nullptr;
    const void* value = nullptr;

    template <typename T>
    static PLY_NO_INLINE void format_thunk(OutStream& out, const void* arg) {
        fmt::TypePrinter<T>::print(out, *(const T*) arg);
    }
    static PLY_INLINE void collect_internal(FormatArg*) {
    }
    template <typename T, typename... Rest>
    static PLY_INLINE void collect_internal(FormatArg* argList, const T& arg,
                                            const Rest&... rest) {
        argList->func = format_thunk<T>;
        argList->value = &arg;
        collect_internal(argList + 1, rest...);
    }
    template <typename... Args>
    static PLY_INLINE auto collect(const Args&... args) {
        FixedArray<FormatArg, sizeof...(args)> argList;
        collect_internal(argList.items, args...);
        return argList;
    }
};

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
        if (this->end_byte < this->cur_byte)
            return true;
        return this->make_writable();
    }
    bool write(char c) {
        if (this->cur_byte >= this->end_byte) {
            if (!this->make_writable())
                return false;
        }
        *this->cur_byte++ = c;
        return true;
    }
    bool write(StringView src);
    template <typename T>
    void raw_write(const T& value) {
        this->write({(const char*) &value, sizeof(T)});
    }
    OutStream& operator<<(char c) {
        if (this->cur_byte >= this->end_byte) {
            if (!this->make_writable())
                return *this;
        }
        *this->cur_byte++ = c;
        return *this;
    }
    OutStream& operator<<(StringView buf) {
        this->write(buf);
        return *this;
    }
    template <typename T>
    PLY_NO_INLINE OutStream& operator<<(const T& value) {
        fmt::TypePrinter<T>::print(*this, value);
        return *this;
    }
    void format_args(StringView fmt, ArrayView<const FormatArg> args);
    template <typename... Args>
    PLY_NO_INLINE void format(StringView fmt, const Args&... args) {
        auto argList = FormatArg::collect(args...);
        this->format_args(fmt, argList);
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
        out.write({(const char*) &value, sizeof(value)});
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
PLY_INLINE String String::from(const T& value) {
    MemOutStream mout;
    mout << value;
    return mout.moveToString();
}

} // namespace ply

#include <ply-runtime/io/impl/FormatString.h>
