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

namespace ply {

//  ▄▄▄▄         ▄▄▄▄   ▄▄
//   ██  ▄▄▄▄▄  ██  ▀▀ ▄██▄▄ ▄▄▄▄▄   ▄▄▄▄   ▄▄▄▄  ▄▄▄▄▄▄▄
//   ██  ██  ██  ▀▀▀█▄  ██   ██  ▀▀ ██▄▄██  ▄▄▄██ ██ ██ ██
//  ▄██▄ ██  ██ ▀█▄▄█▀  ▀█▄▄ ██     ▀█▄▄▄  ▀█▄▄██ ██ ██ ██
//

struct ViewInStream;

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

//------------------------------------------------------------------
// NativeEndianReader
//------------------------------------------------------------------
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

} // namespace ply

#include <ply-runtime/io/impl/TypeParser.h>
