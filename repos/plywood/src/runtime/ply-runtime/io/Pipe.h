/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/string/StringView.h>

namespace ply {

enum class SeekDir {
    Set,
    Cur,
    End,
};

//------------------------------------------------------------------------------------------------
/*!
`InPipe` is a class for reading input from an arbitrary data source such as a file, network socket,
or encryption/compression codec. It's often used as the underlying input source of an `InStream`.

`InPipe` is a lower-level class than `InStream` and is more akin to a Unix file descriptor. When the
`InPipe` reads from a file or network socket, each call to `readSome()` or `read()` invokes a system
call to the OS kernel. Therefore, when reading directly from an `InPipe`, it's best to read
medium-to-large quantities of data (say, several KB) at a time in order to minimize system call
overhead.

If reading small amounts of data (say, a few bytes) at a time -- for example, to parse text or
binary formats -- it's better to work with `InStream` or `StringReader` instead.
*/
struct InPipe {
    // FIXME: Make these virtual?
    struct Funcs {
        void (*destroy)(InPipe*) = nullptr;
        u32 (*readSome)(InPipe*, MutableStringView) = nullptr;
        u64 (*getFileSize)(const InPipe*) = nullptr;
    };

    Funcs* funcs = nullptr;

    PLY_INLINE InPipe(Funcs* funcs) : funcs{funcs} {
    }
    PLY_INLINE ~InPipe() {
        this->funcs->destroy(this);
    }

    /*!
    Cast this `InPipe` to a derived type such as `InPipe_Win32`, `InPipe_FD` or `InPipe_Winsock` in
    order to access additional functions or data members. A runtime check is performed to ensure the
    cast is valid.
    */
    template <typename T>
    PLY_INLINE T* cast() {
        PLY_ASSERT(this->funcs == &T::Funcs_);
        return static_cast<T*>(this);
    }

    /*!
    Attempts to read some data into `buf` from the input source. If the `InPipe` is waiting for
    data, this function will block until some data arrives. Returns the actual number of bytes read,
    which might be less than the size of `buf`. Returns 0 if EOF/error is encountered.
    */
    PLY_INLINE u32 readSome(MutableStringView buf) {
        return this->funcs->readSome(this, buf);
    }

    /*!
    Attempts to completely fill `buf` with data from the input source. If the `InPipe` is waiting
    for data, this function will block until `buf` is completely filled. Returns `true` if
    successful. [FIXME: Reconsider how incomplete reads are handled]
    */
    PLY_DLL_ENTRY bool read(MutableStringView buf);

    /*!
    If the `InPipe` is reading from a file, returns the size of the file in bytes.
    */
    PLY_INLINE u64 getFileSize() const {
        return this->funcs->getFileSize(this);
    }

    static u64 getFileSize_Unsupported(const InPipe*);
};

//------------------------------------------------------------------------------------------------
/*!
`OutPipe` is a class for writing output to an arbitrary destination such as a file, network socket,
or encryption/compression codec. It's often used as the underlying output destination of an
`OutStream`.

`OutPipe` is a lower-level class than `OutStream` and is more akin to a Unix file descriptor. When
the `OutPipe` writes to a file or network socket, each call to `write()` invokes a system call to
the OS kernel. Therefore, when writing directly to an `OutPipe`, it's best to write medium-to-large
quantities of data (say, several KB) at a time in order to minimize system call overhead.

If writing small amounts of data (say, a few bytes) at a time -- for example, to parse text or
binary formats -- it's better to work with `OutStream` or `StringWriter` instead.
*/
struct OutPipe {
    struct Funcs {
        void (*destroy)(OutPipe*) = nullptr;
        bool (*write)(OutPipe*, StringView) = nullptr;
        bool (*flush)(OutPipe*, bool) = nullptr;
        u64 (*seek)(OutPipe*, s64, SeekDir) = nullptr;
    };

    Funcs* funcs = nullptr;

    PLY_INLINE OutPipe(Funcs* funcs) : funcs{funcs} {
    }
    PLY_INLINE ~OutPipe() {
        this->funcs->destroy(this);
    }

    /*!
    Cast this `OutPipe` to a derived type such as `OutPipe_Win32`, `OutPipe_FD` or `OutPipe_Winsock`
    in order to access additional functions or data members. A runtime check is performed to ensure
    the cast is valid.
    */
    template <typename T>
    PLY_INLINE T* cast() {
        PLY_ASSERT(this->funcs == &T::Funcs_);
        return static_cast<T*>(this);
    }

    /*!
    Attempts to write the entire contents of `buf` to the output destination. This call may block
    depending on the state of the `OutPipe`. For example, if the `OutPipe` is a network socket or
    interprocess pipe, and the process on the other end is not reading quickly enough, `write()` may
    block. Returns `true` if successful. Return `false` if the write fails such as when attempting
    to write to a network socket that was closed prematurely.
    */
    PLY_INLINE bool write(StringView buf) {
        return this->funcs->write(this, buf);
    }

    /*!
    Flushes any application-level memory buffers in the same manner as `flushMem()`, then performs
    an implementation-specific device flush if `toDevice` is `true`. For example, if `toDevice` is
    `true`, this will call `fsync()` on when writing to a POSIX file descriptor or
    `FlushFileBuffers()` when writing to a Windows file handle. Returns `true` unless there's an
    error in the implementation-specific device flush.
    */
    PLY_INLINE bool flush(bool toDevice = true) {
        return this->funcs->flush(this, toDevice);
    }

    /*!
    Flushes any application-level memory buffers maintained by the `OutPipe` will be flushed to
    their destination. Appropriate to use when the `OutPipe` is a line feed filter or any kind of
    application-level conversion, compression or encryption filter.
    */
    PLY_INLINE void flushMem() {
        this->funcs->flush(this, false);
    }

    /*!
    If the output destination is seekable, seeks to the specified file offset and returns the
    current file offset. To get the current file offset without changing it, call `seek(0,
    SeekDir::Cur)`.
    */
    PLY_INLINE u64 seek(s64 pos, SeekDir seekDir) {
        return this->funcs->seek(this, pos, seekDir);
    }

    static void flush_Empty(OutPipe*);
    static u64 seek_Empty(OutPipe*, s64, SeekDir);
};

} // namespace ply
