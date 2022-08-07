/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-platform/Util.h>
#if PLY_TARGET_WIN32
#include <malloc.h>
#else
#include <stdlib.h>
#endif // PLY_TARGET_WIN32

namespace ply {

class Heap_CRT {
public:
    class Operator {
    public:
        void* alloc(ureg size) {
            return ::malloc((size_t) size);
        }

        void* realloc(void* ptr, ureg newSize) {
            return ::realloc(ptr, newSize);
        }

        void free(void* ptr) {
            ::free(ptr);
        }

        void* allocAligned(ureg size, ureg alignment) {
            PLY_ASSERT(isPowerOf2(alignment));
#if PLY_TARGET_WIN32
            return ::_aligned_malloc((size_t) size, (size_t) alignment);
#else
            void* ptr;
            int rc = posix_memalign(&ptr, max<size_t>(alignment, PLY_PTR_SIZE), (size_t) size);
            PLY_ASSERT(rc == 0);
            PLY_UNUSED(rc);
            return ptr;
#endif // PLY_TARGET_WIN32
        }

        void freeAligned(void* ptr) {
#if PLY_TARGET_WIN32
            ::_aligned_free(ptr);
#else
            ::free(ptr);
#endif // PLY_TARGET_WIN32
        }
    };

    Operator operate(const char*) {
        return Operator();
    }
};

} // namespace ply
