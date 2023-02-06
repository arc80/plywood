/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once

namespace ply {

//  ▄▄▄▄         ▄▄▄▄   ▄▄
//   ██  ▄▄▄▄▄  ██  ▀▀ ▄██▄▄ ▄▄▄▄▄   ▄▄▄▄   ▄▄▄▄  ▄▄▄▄▄▄▄
//   ██  ██  ██  ▀▀▀█▄  ██   ██  ▀▀ ██▄▄██  ▄▄▄██ ██ ██ ██
//  ▄██▄ ██  ██ ▀█▄▄█▀  ▀█▄▄ ██     ▀█▄▄▄  ▀█▄▄██ ██ ██ ██
//

struct InPipe;

struct InStream {
    struct Status {
        u32 block_size : 29;
        u32 is_pipe_owner : 1;
        u32 eof : 1;
        u32 parse_error : 1;
        Status() : block_size{4096}, is_pipe_owner{0}, eof{0}, parse_error{0} {
        }
    };

    const char* start_byte = nullptr;
    const char* cur_byte = nullptr;
    const char* end_byte = nullptr;
    Reference<BlockList::Footer> block; // nullptr if reading from StringView
    InPipe* in_pipe;                    // nullptr if reading from memory
    Status status;

    // ┏━━━━━━━━━━━━━━━━┓
    // ┃  Constructors  ┃
    // ┗━━━━━━━━━━━━━━━━┛
    InStream() = default;
    // in_pipe can be nullptr, in which case is_open() will return false.
    InStream(InPipe* in_pipe, bool is_pipe_owner);
    InStream(Owned<InPipe>&& in_pipe) : InStream{in_pipe.release(), true} {
    }
    InStream(InStream&& other);
    ~InStream();

    void operator=(InStream&& other) {
        PLY_ASSERT(this != &other);
        this->~InStream();
        new (this) InStream{std::move(other)};
    }

    // ┏━━━━━━━━━━━┓
    // ┃  Methods  ┃
    // ┗━━━━━━━━━━━┛
    u64 get_seek_pos() const;
    BlockList::Ref get_save_point() const {
        return {this->block, const_cast<char*>(this->cur_byte)};
    }
    void rewind(const BlockList::WeakRef& pos);

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
    bool any_parse_error() const {
        return this->status.parse_error != 0;
    }
    u32 num_bytes_readable() const {
        return safe_demote<u32>(this->end_byte - this->cur_byte);
    }
    StringView view_readable() const {
        return {this->cur_byte, safe_demote<u32>(this->end_byte - this->cur_byte)};
    }
    bool load_more_data();
    bool ensure_readable() {
        if (this->cur_byte < this->end_byte)
            return true;
        return this->load_more_data();
    }
    bool read_internal(MutStringView dst);
    bool read(MutStringView dst) {
        if (dst.num_bytes > safe_demote<u32>(this->end_byte - this->cur_byte))
            return this->read_internal(dst);
        memcpy(dst.bytes, this->cur_byte, dst.num_bytes);
        this->cur_byte += dst.num_bytes;
        return true;
    }
    template <typename T>
    T raw_read() {
        T result;
        this->read({(char*) &result, sizeof(T)});
        return result;
    }
    char read_byte_internal();
    char read_byte() {
        if (this->cur_byte < this->end_byte)
            return *this->cur_byte++;
        return this->read_byte_internal();
    }

    String read_remaining_contents();

    template <typename Type>
    PLY_INLINE Type parse(const decltype(fmt::TypeParser<Type>::default_format())&
                              format = fmt::TypeParser<Type>::default_format()) {
        return fmt::TypeParser<Type>::parse(*this, format);
    }
    template <typename Format,
              typename = void_t<decltype(fmt::FormatParser<Format>::parse)>>
    PLY_INLINE auto parse(const Format& format = {}) {
        return fmt::FormatParser<Format>::parse(*this, format);
    }
    template <typename Type>
    PLY_INLINE String
    read_string(const decltype(fmt::TypeParser<Type>::default_format())& format =
                    fmt::TypeParser<Type>::default_format()) {
        BlockList::Ref start_pos = this->get_block_ref();
        fmt::TypeParser<Type>::parse(this, format); // ignore return value
        return BlockList::to_string(std::move(start_pos), this->get_block_ref());
    }
    template <typename Format,
              typename = void_t<decltype(fmt::FormatParser<Format>::parse)>>
    PLY_INLINE String read_string(const Format& format = {}) {
        BlockList::Ref start_pos = this->get_block_ref();
        fmt::FormatParser<Format>::parse(this, format); // ignore return value
        return BlockList::to_string(std::move(start_pos), this->get_block_ref());
    }
};

struct ViewInStream : InStream {
    ViewInStream() = default;
    explicit ViewInStream(StringView view);

    PLY_INLINE StringView get_view_from(const BlockList::WeakRef& save_point) const {
        PLY_ASSERT(uptr(this->cur_byte - save_point.byte) <=
                   uptr(this->end_byte - this->start_byte));
        return StringView::from_range(save_point.byte, this->cur_byte);
    }

    template <typename Type>
    PLY_INLINE StringView
    read_view(const decltype(fmt::TypeParser<Type>::default_format())& format =
                  fmt::TypeParser<Type>::default_format()) {
        PLY_ASSERT(!this->block);
        const char* start_byte = (const char*) this->cur_byte;
        fmt::TypeParser<Type>::parse(*this, format); // ignore return value
        return StringView::from_range(start_byte, (const char*) this->cur_byte);
    }

    template <typename Format,
              typename = void_t<decltype(fmt::FormatParser<Format>::parse)>>
    PLY_INLINE StringView read_view(const Format& format = {}) {
        PLY_ASSERT(!this->block);
        const char* start_byte = (const char*) this->cur_byte;
        fmt::FormatParser<Format>::parse(*this, format); // ignore return value
        return StringView::from_range(start_byte, (const char*) this->cur_byte);
    }
};

// ┏━━━━━━━━━━━━━━━━━━━━━━┓
// ┃  NativeEndianReader  ┃
// ┗━━━━━━━━━━━━━━━━━━━━━━┛
class NativeEndianReader {
public:
    InStream& in;

    PLY_INLINE NativeEndianReader(InStream& in) : in{in} {
    }

    template <typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value>>
    PLY_INLINE T read() {
        T value;
        in.read({(char*) &value, sizeof(value)});
        return value;
    }
};

template <typename T>
PLY_NO_INLINE T StringView::to(const T& default_value) const {
    ViewInStream vins{this->trim(is_white)};
    T value = vins.parse<T>();
    if (vins.at_eof() && !vins.any_parse_error()) {
        return value;
    }
    return default_value;
}

//   ▄▄▄▄          ▄▄    ▄▄▄▄   ▄▄
//  ██  ██ ▄▄  ▄▄ ▄██▄▄ ██  ▀▀ ▄██▄▄ ▄▄▄▄▄   ▄▄▄▄   ▄▄▄▄  ▄▄▄▄▄▄▄
//  ██  ██ ██  ██  ██    ▀▀▀█▄  ██   ██  ▀▀ ██▄▄██  ▄▄▄██ ██ ██ ██
//  ▀█▄▄█▀ ▀█▄▄██  ▀█▄▄ ▀█▄▄█▀  ▀█▄▄ ██     ▀█▄▄▄  ▀█▄▄██ ██ ██ ██
//

struct OutPipe;

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
        return safe_demote<u32>(this->end_byte - this->cur_byte);
    }
    MutStringView view_writable() {
        return {cur_byte, safe_demote<u32>(end_byte - cur_byte)};
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
    String move_to_string(); // Must be a MemOutStream.
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
    return mout.move_to_string();
}

template <typename T>
String to_string(T&& value) {
    MemOutStream mout;
    FormatArg{std::forward<T>(value)}.print(mout);
    return mout.move_to_string();
}

//  ▄▄▄▄        ▄▄▄▄▄  ▄▄
//   ██  ▄▄▄▄▄  ██  ██ ▄▄ ▄▄▄▄▄   ▄▄▄▄
//   ██  ██  ██ ██▀▀▀  ██ ██  ██ ██▄▄██
//  ▄██▄ ██  ██ ██     ██ ██▄▄█▀ ▀█▄▄▄
//                        ██

enum SeekDirection {
    Seek_Absolute,
    Seek_Relative,
    Seek_End,
};

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

#elif PLY_TARGET_POSIX

// ┏━━━━━━━━━━━━━┓
// ┃  InPipe_FD  ┃
// ┗━━━━━━━━━━━━━┛
struct InPipe_FD : InPipe {
    static constexpr char* Type = "FD";
    int fd = -1;

    InPipe_Handle(int fd) : InPipe{Type}, fd{fd} {
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

#elif PLY_TARGET_POSIX

// ┏━━━━━━━━━━━━━━┓
// ┃  OutPipe_FD  ┃
// ┗━━━━━━━━━━━━━━┛
struct OutPipe_FD : OutPipe {
    static constexpr char* Type = "FD";
    int fd = -1;

    OutPipe_Handle(int fd) : OutPipe{Type}, fd{fd} {
    }
    virtual ~OutPipe_Handle();
    virtual bool write(StringView buf) override;
    virtual void flush(bool hard) override;
    virtual void seek(s64 offset, SeekDirection dir) override;
};

#endif // PLY_TARGET_WIN32

//   ▄▄▄▄                              ▄▄▄
//  ██  ▀▀  ▄▄▄▄  ▄▄▄▄▄   ▄▄▄▄   ▄▄▄▄   ██   ▄▄▄▄
//  ██     ██  ██ ██  ██ ▀█▄▄▄  ██  ██  ██  ██▄▄██
//  ▀█▄▄█▀ ▀█▄▄█▀ ██  ██  ▄▄▄█▀ ▀█▄▄█▀ ▄██▄ ▀█▄▄▄
//

InPipe* get_console_in_pipe();
OutPipe* get_console_out_pipe();
OutPipe* get_console_error_pipe();

enum ConsoleMode {
    CM_Text,
    CM_Binary,
};

struct Console_t {
    InStream in(ConsoleMode mode = CM_Text);
    OutStream out(ConsoleMode mode = CM_Text);
    OutStream error(ConsoleMode mode = CM_Text);
};

extern Console_t Console;

//  ▄▄▄▄▄
//  ██  ██ ▄▄▄▄▄   ▄▄▄▄   ▄▄▄▄  ▄▄▄▄   ▄▄▄▄   ▄▄▄▄
//  ██▀▀▀  ██  ▀▀ ██  ██ ██    ██▄▄██ ▀█▄▄▄  ▀█▄▄▄
//  ██     ██     ▀█▄▄█▀ ▀█▄▄▄ ▀█▄▄▄   ▄▄▄█▀  ▄▄▄█▀
//

struct Process {
    enum struct Pipe {
        Open,
        Redirect, // This will redirect output to /dev/null if corresponding OutPipe
                  // (std_out_pipe/std_err_pipe) is unopened
        StdOut,
    };

    struct Output {
        Pipe std_out = Pipe::Redirect;
        Pipe std_err = Pipe::Redirect;
        OutPipe* std_out_pipe = nullptr;
        OutPipe* std_err_pipe = nullptr;

        static PLY_INLINE Output ignore() {
            return {};
        }
        static PLY_INLINE Output inherit() {
            Output h;
            h.std_out_pipe = get_console_out_pipe();
            h.std_err_pipe = get_console_error_pipe();
            return h;
        }
        static PLY_INLINE Output open_separate() {
            Output h;
            h.std_out = Pipe::Open;
            h.std_err = Pipe::Open;
            return h;
        }
        static PLY_INLINE Output open_merged() {
            Output h;
            h.std_out = Pipe::Open;
            h.std_err = Pipe::StdOut;
            return h;
        }
        static PLY_INLINE Output open_std_out_only() {
            Output h;
            h.std_out = Pipe::Open;
            return h;
        }
    };

    struct Input {
        Pipe std_in = Pipe::Redirect;
        InPipe* std_in_pipe = nullptr;

        static PLY_INLINE Input ignore() {
            return {};
        }
        static PLY_INLINE Input inherit() {
            return {Pipe::Redirect, get_console_in_pipe()};
        }
        static PLY_INLINE Input open() {
            return {Pipe::Open, nullptr};
        }
    };

    // Members
    Owned<OutPipe> write_to_std_in;
    Owned<InPipe> read_from_std_out;
    Owned<InPipe> read_from_std_err;

#if PLY_TARGET_WIN32
    HANDLE child_process = INVALID_HANDLE_VALUE;
    HANDLE child_main_thread = INVALID_HANDLE_VALUE;
#elif PLY_TARGET_POSIX
    int child_pid = -1;
#endif

    PLY_INLINE Process() = default;
    ~Process();
    s32 join();

    static PLY_DLL_ENTRY Owned<Process>
    exec_arg_str(StringView exe_path, StringView arg_str, StringView initial_dir,
                 const Output& output, const Input& input = Input::open());
    static PLY_DLL_ENTRY Owned<Process>
    exec(StringView exe_path, ArrayView<const StringView> args, StringView initial_dir,
         const Output& output, const Input& input = Input::open());
};

//  ▄▄  ▄▄        ▄▄                  ▄▄
//  ██  ██ ▄▄▄▄▄  ▄▄  ▄▄▄▄  ▄▄▄▄   ▄▄▄██  ▄▄▄▄
//  ██  ██ ██  ██ ██ ██    ██  ██ ██  ██ ██▄▄██
//  ▀█▄▄█▀ ██  ██ ██ ▀█▄▄▄ ▀█▄▄█▀ ▀█▄▄██ ▀█▄▄▄
//

enum UnicodeType {
    NotUnicode,
    UTF8,
    UTF16_Native,
    UTF16_Reversed,
#if PLY_IS_BIG_ENDIAN
    UTF16_LE = UTF16_Reversed,
    UTF16_BE = UTF16_Native,
#else
    UTF16_LE = UTF16_Native,
    UTF16_BE = UTF16_Reversed,
#endif
};

struct ExtendedTextParams {
    ArrayView<s32> lut; // Lookup table: byte -> Unicode codepoint.
    Map<u32, u8> reverse_lut;
    s32 missing_char = 255; // If negative, missing characters are skipped.
};

enum DecodeStatus {
    DS_OK,
    DS_IllFormed,     // Not at EOF.
    DS_NotEnoughData, // Can still decode an ill-formed codepoint.
};

struct Unicode {
    UnicodeType type;
    ExtendedTextParams* ext_params = nullptr;
    DecodeStatus status = DS_OK;

    Unicode(UnicodeType type = NotUnicode) : type{type} {
    }

    bool encode_point(OutStream& out, u32 codepoint);
    s32 decode_point(InStream& in); // -1 at EOF
};

// ┏━━━━━━━━━━━━━━━━━━━━━━━━━┓
// ┃  InPipe_ConvertUnicode  ┃
// ┗━━━━━━━━━━━━━━━━━━━━━━━━━┛
struct InPipe_ConvertUnicode : InPipe {
    InStream in;
    Unicode src_enc;

    // shim_storage is used to split multibyte characters at buffer boundaries.
    FixedArray<char, 4> shim_storage;
    StringView shim_used;

    InPipe_ConvertUnicode(InStream&& in, UnicodeType type = NotUnicode)
        : in{std::move(in)}, src_enc{type} {
    }

    // Fill dst_buf with UTF-8-encoded data.
    virtual u32 read(MutStringView dst_buf) override;
};

// ┏━━━━━━━━━━━━━━━━━━━━━━━━━━┓
// ┃  OutPipe_ConvertUnicode  ┃
// ┗━━━━━━━━━━━━━━━━━━━━━━━━━━┛
struct OutPipe_ConvertUnicode : OutPipe {
    Unicode dst_enc;

    // shim_storage is used to join multibyte characters at buffer boundaries.
    char shim_storage[4];
    u32 shim_used;

    OutPipe_ConvertUnicode(OutStream&& out, UnicodeType type = NotUnicode)
        : dst_enc{type} {
        this->child_stream = std::move(out);
    }

    // src_buf expects UTF-8-encoded data.
    virtual bool write(StringView src_buf) override;
    virtual void flush(bool hard) override;
};

//  ▄▄▄▄▄▄                ▄▄   ▄▄▄▄▄                                ▄▄
//    ██    ▄▄▄▄  ▄▄  ▄▄ ▄██▄▄ ██     ▄▄▄▄  ▄▄▄▄▄  ▄▄▄▄▄▄▄   ▄▄▄▄  ▄██▄▄
//    ██   ██▄▄██  ▀██▀   ██   ██▀▀  ██  ██ ██  ▀▀ ██ ██ ██  ▄▄▄██  ██
//    ██   ▀█▄▄▄  ▄█▀▀█▄  ▀█▄▄ ██    ▀█▄▄█▀ ██     ██ ██ ██ ▀█▄▄██  ▀█▄▄
//

struct TextFormat {
    enum class NewLine {
        LF,
        CRLF,
    };
    static constexpr u32 NumBytesForAutodetect = 4000;

    UnicodeType encoding = UTF8;
    NewLine new_line = NewLine::LF;
    bool bom = true;

    static TextFormat default_utf8();
    static TextFormat autodetect(InStream& in);

    Owned<InPipe> create_importer(InStream&& in) const;
    Owned<OutPipe> create_exporter(OutStream&& out) const;

    PLY_INLINE bool operator==(const TextFormat& other) const {
        return (this->encoding == other.encoding) &&
               (this->new_line == other.new_line) && (this->bom == other.bom);
    }
};

//  ▄▄▄▄▄          ▄▄   ▄▄
//  ██  ██  ▄▄▄▄  ▄██▄▄ ██▄▄▄
//  ██▀▀▀   ▄▄▄██  ██   ██  ██
//  ██     ▀█▄▄██  ▀█▄▄ ██  ██
//

struct Path_t {
    // On Windows:
    //  \ is the default separator
    //  / and \ are both recognized as separators
    //  (eg. CMake-style path such as "C:/path/to/file" can be manipulated as a Windows
    //  path) Drive letters are recognized when joining paths Absolute paths must begin
    //  with a drive letter (FIXME: Should also recognize UNC paths, eg. \\server\share,
    //  as absolute)
    //
    // On POSIX:
    //  / is the only separator
    //  Windows-style drive letters are treated like any other path component

    bool is_windows = false;

    PLY_INLINE bool is_sep_byte(char c) const {
        return c == '/' || (this->is_windows && c == '\\');
    }

    PLY_INLINE char sep_byte() const {
        return this->is_windows ? '\\' : '/';
    }

    PLY_INLINE bool has_drive_letter(StringView path) const {
        if (!this->is_windows)
            return false;
        return path.num_bytes >= 2 && is_ascii_letter(path.bytes[0]) &&
               path.bytes[1] == ':';
    }

    PLY_INLINE StringView get_drive_letter(StringView path) const {
        return has_drive_letter(path) ? path.left(2) : StringView{};
    }

    PLY_INLINE bool is_absolute(StringView path) const {
        if (this->is_windows) {
            return path.num_bytes >= 3 && this->has_drive_letter(path) &&
                   this->is_sep_byte(path[2]);
        } else {
            return path.num_bytes >= 1 && this->is_sep_byte(path[0]);
        }
    }

    PLY_INLINE bool is_relative(StringView path) const {
        return !this->has_drive_letter(path) && path.num_bytes > 0 &&
               !this->is_sep_byte(path[0]);
    }

    PLY_DLL_ENTRY Tuple<StringView, StringView> split(StringView path) const;
    PLY_DLL_ENTRY Array<StringView> split_full(StringView path) const;
    PLY_DLL_ENTRY Tuple<StringView, StringView> split_ext(StringView path) const;
    PLY_DLL_ENTRY String join_array(ArrayView<const StringView> components) const;

    template <typename... StringViews>
    PLY_INLINE String join(StringViews&&... path_component_args) const {
        FixedArray<StringView, sizeof...(StringViews)> components{
            std::forward<StringViews>(path_component_args)...};
        return join_array(components);
    }

    PLY_DLL_ENTRY String make_relative(StringView ancestor,
                                       StringView descendant) const;
    PLY_DLL_ENTRY HybridString from(const Path_t& src_format,
                                    StringView src_path) const;

    PLY_INLINE bool ends_with_sep(StringView path) {
        return path.num_bytes > 0 && this->is_sep_byte(path.back());
    }
};

struct WString;
WString win32_path_arg(StringView path, bool allow_extended = true);

extern Path_t Path;
extern Path_t WindowsPath;
extern Path_t PosixPath;

String get_workspace_path();

//  ▄▄▄▄▄ ▄▄ ▄▄▄          ▄▄▄▄                 ▄▄
//  ██    ▄▄  ██   ▄▄▄▄  ██  ▀▀ ▄▄  ▄▄  ▄▄▄▄  ▄██▄▄  ▄▄▄▄  ▄▄▄▄▄▄▄
//  ██▀▀  ██  ██  ██▄▄██  ▀▀▀█▄ ██  ██ ▀█▄▄▄   ██   ██▄▄██ ██ ██ ██
//  ██    ██ ▄██▄ ▀█▄▄▄  ▀█▄▄█▀ ▀█▄▄██  ▄▄▄█▀  ▀█▄▄ ▀█▄▄▄  ██ ██ ██
//                               ▄▄▄█▀

enum class FSResult {
    Unknown = 0,
    NotFound,
    Locked,
    AccessDenied,
    OK,
    AlreadyExists,
    Unchanged,
};

enum class ExistsResult {
    NotFound,
    File,
    Directory,
};

struct FileInfo {
    FSResult result = FSResult::Unknown; // Result of get_file_info()
    String name;
    bool is_dir = false;
    u64 file_size = 0;            // Size of the file in bytes
    double creation_time = 0;     // The file's POSIX creation time
    double access_time = 0;       // The file's POSIX access time
    double modification_time = 0; // The file's POSIX modification time
};

struct WalkTriple {
    String dir_path;
    Array<String> dir_names;
    Array<FileInfo> files;
};

struct FileSystemIface;

class FileSystemWalker {
private:
    struct StackItem {
        String path;
        Array<String> dir_names;
        u32 dir_index;
    };

    WalkTriple triple;
    Array<StackItem> stack;
    FileSystemIface* fs = nullptr;
    u32 flags = 0;

    friend struct FileSystemIface;
    void visit(StringView dir_path);

public:
    PLY_INLINE FileSystemWalker() = default;
    PLY_NO_INLINE FileSystemWalker(FileSystemWalker&&) = default;

    // Range-for support:
    struct Iterator {
        FileSystemWalker* walker;
        PLY_INLINE WalkTriple& operator*() {
            return this->walker->triple;
        }
        void operator++();
        PLY_INLINE bool operator!=(const Iterator&) const {
            return !this->walker->triple.dir_path.is_empty();
        }
    };
    PLY_INLINE Iterator begin() {
        return {this};
    }
    PLY_INLINE Iterator end() {
        return {this};
    }
};

// FileSystemIface allows us to create virtual filesystems.
struct FileSystemIface {
    static const u32 WithSizes = 0x1;
    static const u32 WithTimes = 0x2;

    static ThreadLocal<FSResult> last_result_;

    static PLY_INLINE FSResult set_last_result(FSResult result) {
        FileSystemIface::last_result_.store(result);
        return result;
    }
    static PLY_INLINE FSResult last_result() {
        return FileSystemIface::last_result_.load();
    }

    virtual ~FileSystemIface() {
    }
    virtual Path_t path_format() = 0;
    virtual FSResult set_working_directory(StringView path) = 0;
    virtual String get_working_directory() = 0;
    virtual ExistsResult exists(StringView path) = 0;
    virtual Array<FileInfo> list_dir(StringView path,
                                     u32 flags = WithSizes | WithTimes) = 0;
    virtual FSResult make_dir(StringView path) = 0;
    virtual FSResult move_file(StringView src_path, StringView dst_path) = 0;
    virtual FSResult delete_file(StringView path) = 0;
    virtual FSResult remove_dir_tree(StringView dir_path) = 0;
    virtual FileInfo get_file_info(StringView path) = 0; // Doesn't set FileInfo::name
    virtual Owned<InPipe> open_pipe_for_read(StringView path) = 0;
    virtual Owned<OutPipe> open_pipe_for_write(StringView path) = 0;

    PLY_INLINE bool is_dir(StringView path) {
        return this->exists(path) == ExistsResult::Directory;
    }

    FileSystemWalker walk(StringView top, u32 flags = WithSizes | WithTimes);
    FSResult make_dirs(StringView path);
    InStream open_stream_for_read(StringView path);
    OutStream open_stream_for_write(StringView path);
    InStream open_text_for_read(StringView path,
                                const TextFormat& format = TextFormat::default_utf8());
    InStream open_text_for_read_autodetect(StringView path,
                                           TextFormat* out_format = nullptr);
    OutStream
    open_text_for_write(StringView path,
                        const TextFormat& format = TextFormat::default_utf8());
    String load_binary(StringView path);
    String load_text(StringView path, const TextFormat& format);
    String load_text_autodetect(StringView path, TextFormat* out_format = nullptr);
    FSResult make_dirs_and_save_binary_if_different(StringView path,
                                                    StringView contents);
    FSResult make_dirs_and_save_text_if_different(
        StringView path, StringView str_contents,
        const TextFormat& format = TextFormat::default_utf8());
};

struct FileSystem_t : FileSystemIface {
#if PLY_TARGET_WIN32
    // ReadWriteLock used to mitigate data race issues with SetCurrentDirectoryW:
    // https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-setcurrentdirectory
    ReadWriteLock working_dir_lock;

    // Direct access to Windows handles:
    HANDLE open_handle_for_read(StringView path);
    HANDLE open_handle_for_write(StringView path);
    FileInfo get_file_info(HANDLE handle);
#endif

    virtual Array<FileInfo> list_dir(StringView path,
                                     u32 flags = WithSizes | WithTimes) override;
    virtual FSResult make_dir(StringView path) override;
    virtual Path_t path_format() override;
    virtual String get_working_directory() override;
    virtual FSResult set_working_directory(StringView path) override;
    virtual ExistsResult exists(StringView path) override;
    virtual Owned<InPipe> open_pipe_for_read(StringView path) override;
    virtual Owned<OutPipe> open_pipe_for_write(StringView path) override;
    virtual FSResult move_file(StringView src_path, StringView dst_path) override;
    virtual FSResult delete_file(StringView path) override;
    virtual FSResult remove_dir_tree(StringView dir_path) override;
    virtual FileInfo get_file_info(StringView path) override;

    virtual ~FileSystem_t() {
    }
};

extern FileSystem_t FileSystem;

} // namespace ply

#include <ply-runtime/io/impl/TypeParser.h>
