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
    bool anyParseError() const {
        return this->status.parse_error != 0;
    }
    u32 num_bytes_readable() const {
        return safeDemote<u32>(this->end_byte - this->cur_byte);
    }
    StringView view_readable() const {
        return {this->cur_byte, safeDemote<u32>(this->end_byte - this->cur_byte)};
    }
    bool load_more_data();
    bool ensure_readable() {
        if (this->cur_byte < this->end_byte)
            return true;
        return this->load_more_data();
    }
    bool read_internal(MutStringView dst);
    bool read(MutStringView dst) {
        if (dst.numBytes > safeDemote<u32>(this->end_byte - this->cur_byte))
            return this->read_internal(dst);
        memcpy(dst.bytes, this->cur_byte, dst.numBytes);
        this->cur_byte += dst.numBytes;
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
    PLY_INLINE Type parse(const decltype(fmt::TypeParser<Type>::defaultFormat())&
                              format = fmt::TypeParser<Type>::defaultFormat()) {
        return fmt::TypeParser<Type>::parse(*this, format);
    }
    template <typename Format,
              typename = void_t<decltype(fmt::FormatParser<Format>::parse)>>
    PLY_INLINE auto parse(const Format& format = {}) {
        return fmt::FormatParser<Format>::parse(*this, format);
    }
    template <typename Type>
    PLY_INLINE String readString(const decltype(fmt::TypeParser<Type>::defaultFormat())&
                                     format = fmt::TypeParser<Type>::defaultFormat()) {
        BlockList::Ref startPos = this->getBlockRef();
        fmt::TypeParser<Type>::parse(this, format); // ignore return value
        return BlockList::toString(std::move(startPos), this->getBlockRef());
    }
    template <typename Format,
              typename = void_t<decltype(fmt::FormatParser<Format>::parse)>>
    PLY_INLINE String readString(const Format& format = {}) {
        BlockList::Ref startPos = this->getBlockRef();
        fmt::FormatParser<Format>::parse(this, format); // ignore return value
        return BlockList::toString(std::move(startPos), this->getBlockRef());
    }
};

struct ViewInStream : InStream {
    ViewInStream() = default;
    explicit ViewInStream(StringView view);

    PLY_INLINE StringView getViewFrom(const BlockList::WeakRef& savePoint) const {
        PLY_ASSERT(uptr(this->cur_byte - savePoint.byte) <=
                   uptr(this->end_byte - this->start_byte));
        return StringView::fromRange(savePoint.byte, this->cur_byte);
    }

    template <typename Type>
    PLY_INLINE StringView
    readView(const decltype(fmt::TypeParser<Type>::defaultFormat())& format =
                 fmt::TypeParser<Type>::defaultFormat()) {
        PLY_ASSERT(!this->block);
        const char* startByte = (const char*) this->cur_byte;
        fmt::TypeParser<Type>::parse(*this, format); // ignore return value
        return StringView::fromRange(startByte, (const char*) this->cur_byte);
    }

    template <typename Format,
              typename = void_t<decltype(fmt::FormatParser<Format>::parse)>>
    PLY_INLINE StringView readView(const Format& format = {}) {
        PLY_ASSERT(!this->block);
        const char* startByte = (const char*) this->cur_byte;
        fmt::FormatParser<Format>::parse(*this, format); // ignore return value
        return StringView::fromRange(startByte, (const char*) this->cur_byte);
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
PLY_NO_INLINE T StringView::to(const T& defaultValue) const {
    ViewInStream vins{this->trim(isWhite)};
    T value = vins.parse<T>();
    if (vins.at_eof() && !vins.anyParseError()) {
        return value;
    }
    return defaultValue;
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
                  // (stdOutPipe/stdErrPipe) is unopened
        StdOut,
    };

    struct Output {
        Pipe stdOut = Pipe::Redirect;
        Pipe stdErr = Pipe::Redirect;
        OutPipe* stdOutPipe = nullptr;
        OutPipe* stdErrPipe = nullptr;

        static PLY_INLINE Output ignore() {
            return {};
        }
        static PLY_INLINE Output inherit() {
            Output h;
            h.stdOutPipe = get_console_out_pipe();
            h.stdErrPipe = get_console_error_pipe();
            return h;
        }
        static PLY_INLINE Output openSeparate() {
            Output h;
            h.stdOut = Pipe::Open;
            h.stdErr = Pipe::Open;
            return h;
        }
        static PLY_INLINE Output openMerged() {
            Output h;
            h.stdOut = Pipe::Open;
            h.stdErr = Pipe::StdOut;
            return h;
        }
        static PLY_INLINE Output openStdOutOnly() {
            Output h;
            h.stdOut = Pipe::Open;
            return h;
        }
    };

    struct Input {
        Pipe stdIn = Pipe::Redirect;
        InPipe* stdInPipe = nullptr;

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
    Owned<OutPipe> writeToStdIn;
    Owned<InPipe> readFromStdOut;
    Owned<InPipe> readFromStdErr;

#if PLY_TARGET_WIN32
    HANDLE childProcess = INVALID_HANDLE_VALUE;
    HANDLE childMainThread = INVALID_HANDLE_VALUE;
#elif PLY_TARGET_POSIX
    int childPID = -1;
#endif

    PLY_INLINE Process() = default;
    ~Process();
    s32 join();

    static PLY_DLL_ENTRY Owned<Process> execArgStr(StringView exePath, StringView argStr,
                                                      StringView initialDir, const Output& output,
                                                      const Input& input = Input::open());
    static PLY_DLL_ENTRY Owned<Process> exec(StringView exePath,
                                                ArrayView<const StringView> args,
                                                StringView initialDir, const Output& output,
                                                const Input& input = Input::open());
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
    struct ReverseLUTTraits {
        using Key = u32;
        struct Item {
            u32 key;
            u8 value;
        };
        static bool match(const Item& item, Key key) {
            return item.key == key;
        }
    };

    ArrayView<s32> lut; // Lookup table: byte -> Unicode codepoint.
    HashMap<ReverseLUTTraits> reverse_lut;
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
    NewLine newLine = NewLine::LF;
    bool bom = false;

    static TextFormat default_utf8();
    static TextFormat autodetect(InStream& in);

    Owned<InPipe> createImporter(InStream&& in) const;
    Owned<OutPipe> createExporter(OutStream&& out) const;

    PLY_INLINE bool operator==(const TextFormat& other) const {
        return (this->encoding == other.encoding) && (this->newLine == other.newLine) &&
               (this->bom == other.bom);
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
    //  (eg. CMake-style path such as "C:/path/to/file" can be manipulated as a Windows path)
    //  Drive letters are recognized when joining paths
    //  Absolute paths must begin with a drive letter
    //  (FIXME: Should also recognize UNC paths, eg. \\server\share, as absolute)
    //
    // On POSIX:
    //  / is the only separator
    //  Windows-style drive letters are treated like any other path component

    bool isWindows = false;

    PLY_INLINE bool isSepByte(char c) const {
        return c == '/' || (this->isWindows && c == '\\');
    }

    PLY_INLINE char sepByte() const {
        return this->isWindows ? '\\' : '/';
    }

    PLY_INLINE bool hasDriveLetter(StringView path) const {
        if (!this->isWindows)
            return false;
        return path.numBytes >= 2 && isAsciiLetter(path.bytes[0]) && path.bytes[1] == ':';
    }

    PLY_INLINE StringView getDriveLetter(StringView path) const {
        return hasDriveLetter(path) ? path.left(2) : StringView{};
    }

    PLY_INLINE bool isAbsolute(StringView path) const {
        if (this->isWindows) {
            return path.numBytes >= 3 && this->hasDriveLetter(path) && this->isSepByte(path[2]);
        } else {
            return path.numBytes >= 1 && this->isSepByte(path[0]);
        }
    }

    PLY_INLINE bool isRelative(StringView path) const {
        return !this->hasDriveLetter(path) && path.numBytes > 0 && !this->isSepByte(path[0]);
    }

    PLY_DLL_ENTRY Tuple<StringView, StringView> split(StringView path) const;
    PLY_DLL_ENTRY Array<StringView> splitFull(StringView path) const;
    PLY_DLL_ENTRY Tuple<StringView, StringView> splitExt(StringView path) const;
    PLY_DLL_ENTRY String joinArray(ArrayView<const StringView> components) const;

    template <typename... StringViews>
    PLY_INLINE String join(StringViews&&... pathComponentArgs) const {
        FixedArray<StringView, sizeof...(StringViews)> components{
            std::forward<StringViews>(pathComponentArgs)...};
        return joinArray(components);
    }

    PLY_DLL_ENTRY String makeRelative(StringView ancestor, StringView descendant) const;
    PLY_DLL_ENTRY HybridString from(const Path_t& srcFormat, StringView srcPath) const;

    PLY_INLINE bool endsWithSep(StringView path) {
        return path.numBytes > 0 && this->isSepByte(path.back());
    }
};

struct WString;
WString win32PathArg(StringView path, bool allowExtended = true);

extern Path_t Path;
extern Path_t WindowsPath;
extern Path_t PosixPath;

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
    FSResult result = FSResult::Unknown; // Result of getFileInfo()
    String name;
    bool isDir = false;
    u64 fileSize = 0;            // Size of the file in bytes
    double creationTime = 0;     // The file's POSIX creation time
    double accessTime = 0;       // The file's POSIX access time
    double modificationTime = 0; // The file's POSIX modification time
};

struct WalkTriple {
    String dirPath;
    Array<String> dirNames;
    Array<FileInfo> files;
};

struct FileSystemIface;

class FileSystemWalker {
private:
    struct StackItem {
        String path;
        Array<String> dirNames;
        u32 dirIndex;
    };

    WalkTriple triple;
    Array<StackItem> stack;
    FileSystemIface* fs = nullptr;
    u32 flags = 0;

    friend struct FileSystemIface;
    void visit(StringView dirPath);

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
            return !this->walker->triple.dirPath.isEmpty();
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

    static ThreadLocal<FSResult> lastResult_;

    static PLY_INLINE FSResult setLastResult(FSResult result) {
        FileSystemIface::lastResult_.store(result);
        return result;
    }
    static PLY_INLINE FSResult lastResult() {
        return FileSystemIface::lastResult_.load();
    }

    virtual ~FileSystemIface() {
    }
    virtual Path_t pathFormat() = 0;
    virtual FSResult setWorkingDirectory(StringView path) = 0;
    virtual String getWorkingDirectory() = 0;
    virtual ExistsResult exists(StringView path) = 0;
    virtual Array<FileInfo> listDir(StringView path,
                                    u32 flags = WithSizes | WithTimes) = 0;
    virtual FSResult makeDir(StringView path) = 0;
    virtual FSResult moveFile(StringView srcPath, StringView dstPath) = 0;
    virtual FSResult deleteFile(StringView path) = 0;
    virtual FSResult removeDirTree(StringView dirPath) = 0;
    virtual FileInfo getFileInfo(StringView path) = 0; // Doesn't set FileInfo::name
    virtual Owned<InPipe> openPipeForRead(StringView path) = 0;
    virtual Owned<OutPipe> openPipeForWrite(StringView path) = 0;

    PLY_INLINE bool isDir(StringView path) {
        return this->exists(path) == ExistsResult::Directory;
    }

    FileSystemWalker walk(StringView top, u32 flags = WithSizes | WithTimes);
    FSResult makeDirs(StringView path);
    InStream openStreamForRead(StringView path);
    OutStream openStreamForWrite(StringView path);
    InStream openTextForRead(StringView path,
                             const TextFormat& format = TextFormat::default_utf8());
    InStream openTextForReadAutodetect(StringView path,
                                       TextFormat* out_format = nullptr);
    OutStream openTextForWrite(StringView path,
                               const TextFormat& format = TextFormat::default_utf8());
    String loadBinary(StringView path);
    String loadText(StringView path, const TextFormat& format);
    String loadTextAutodetect(StringView path, TextFormat* out_format = nullptr);
    FSResult makeDirsAndSaveBinaryIfDifferent(StringView path, StringView contents);
    FSResult makeDirsAndSaveTextIfDifferent(
        StringView path, StringView strContents,
        const TextFormat& format = TextFormat::default_utf8());
};

struct FileSystem_t : FileSystemIface {
#if PLY_TARGET_WIN32
    // ReadWriteLock used to mitigate data race issues with SetCurrentDirectoryW:
    // https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-setcurrentdirectory
    ReadWriteLock workingDirLock;

    // Direct access to Windows handles:
    HANDLE openHandleForRead(StringView path);
    HANDLE openHandleForWrite(StringView path);
    FileInfo getFileInfo(HANDLE handle);
#endif

    virtual Array<FileInfo> listDir(StringView path,
                                    u32 flags = WithSizes | WithTimes) override;
    virtual FSResult makeDir(StringView path) override;
    virtual Path_t pathFormat() override;
    virtual String getWorkingDirectory() override;
    virtual FSResult setWorkingDirectory(StringView path) override;
    virtual ExistsResult exists(StringView path) override;
    virtual Owned<InPipe> openPipeForRead(StringView path) override;
    virtual Owned<OutPipe> openPipeForWrite(StringView path) override;
    virtual FSResult moveFile(StringView srcPath, StringView dstPath) override;
    virtual FSResult deleteFile(StringView path) override;
    virtual FSResult removeDirTree(StringView dirPath) override;
    virtual FileInfo getFileInfo(StringView path) override;

    virtual ~FileSystem_t() {
    }
};

extern FileSystem_t FileSystem;

} // namespace ply

#include <ply-runtime/io/impl/TypeParser.h>
