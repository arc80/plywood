/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                   ┃
┃    ╱   ╱╲    Plywood Multimedia Toolkit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/          ┃
┃    └──┴┴┴┘                                 ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/

#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/container/BlockList.h>
#include <ply-runtime/container/Owned.h>
#include <ply-runtime/string/String.h>
#include <ply-runtime/container/FixedArray.h>

namespace ply {

struct OutPipe;
struct OutStream;

#define PLY_ENABLE_IF_SIGNED(T) \
    typename std::enable_if_t<std::is_signed<T>::value, int> = 0
#define PLY_ENABLE_IF_UNSIGNED(T) \
    typename std::enable_if_t< \
        std::is_unsigned<T>::value && !std::is_same<T, bool>::value, int> = 0

struct FormatArg {
    enum Type {
        View,
        Bool,
        S64,
        U64,
        Double,
    };

    static void default_print(OutStream& out, const FormatArg& arg);
    void (*print_func)(OutStream& out, const FormatArg& arg) = default_print;
    union {
        StringView view;
        bool bool_;
        s64 s64_;
        u64 u64_;
        double double_;
    };
    Type type = View;
    u32 radix = 0;

    FormatArg(StringView view = {}) : view{view} {
    }
    template <typename T,
              typename std::enable_if_t<std::is_same<T, bool>::value, int> = 0>
    FormatArg(T v) : bool_{v}, type{Bool} {
    }
    template <typename T, PLY_ENABLE_IF_SIGNED(T)>
    FormatArg(T v) : s64_{v}, type{S64} {
    }
    template <typename T, PLY_ENABLE_IF_UNSIGNED(T)>
    FormatArg(T v) : u64_{v}, type{U64} {
    }
    FormatArg(double v) : double_{v}, type{Double} {
    }
    void print(OutStream& out) const {
        this->print_func(out, *this);
    }
};

struct with_radix : FormatArg {
    template <typename T, PLY_ENABLE_IF_SIGNED(T)>
    with_radix(T v, u32 radix) : FormatArg{v} {
        this->radix = radix;
    }
    template <typename T, PLY_ENABLE_IF_UNSIGNED(T)>
    with_radix(T v, u32 radix) : FormatArg{v} {
        this->radix = radix;
    }
    with_radix(double v, u32 radix) : FormatArg{v} {
        this->radix = radix;
    }
};

struct hex : FormatArg {
    template <typename T, PLY_ENABLE_IF_SIGNED(T)>
    hex(T v) : FormatArg{v} {
        this->radix = 16;
    }
    template <typename T, PLY_ENABLE_IF_UNSIGNED(T)>
    hex(T v) : FormatArg{v} {
        this->radix = 16;
    }
};

struct escape : FormatArg {
    static void do_print(OutStream& out, const FormatArg& arg);
    escape(StringView view) {
        this->print_func = do_print;
        this->view = view;
    }
};

struct xml_escape : FormatArg {
    static void do_print(OutStream& out, const FormatArg& arg);
    xml_escape(StringView view) {
        this->print_func = do_print;
        this->view = view;
    }
};

struct CmdLineArg_WinCrt : FormatArg {
    static void do_print(OutStream& out, const FormatArg& arg);
    CmdLineArg_WinCrt(StringView view) {
        this->print_func = do_print;
        this->view = view;
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
String to_string(T&& value) {
    MemOutStream mout;
    FormatArg{std::forward<T>(value)}.print(mout);
    return mout.moveToString();
}

} // namespace ply

#include <ply-runtime/io/impl/FormatString.h>
