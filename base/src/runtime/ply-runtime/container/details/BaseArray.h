/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>

namespace ply {
namespace details {

//---------------------------------------------------
// BaseArray
//---------------------------------------------------
struct BaseArray {
    void* m_items = nullptr;
    u32 m_numItems = 0;
    u32 m_allocated = 0;

    PLY_INLINE BaseArray() = default;

    PLY_DLL_ENTRY void alloc(u32 numItems, u32 itemSize);
    PLY_DLL_ENTRY void realloc(u32 numItems, u32 itemSize);
    PLY_DLL_ENTRY void free();
    PLY_DLL_ENTRY void reserve(u32 numItems, u32 itemSize); // m_numItems is unaffected
    PLY_DLL_ENTRY void reserveIncrement(u32 itemSize);      // m_numItems is unaffected
    PLY_DLL_ENTRY void truncate(u32 itemSize);
};

} // namespace details
} // namespace ply
