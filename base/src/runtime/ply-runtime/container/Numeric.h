/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>

namespace ply {

//----------------------------------------------------------------
// Small container able to hold any kind of primitive numeric type.
//
// Mainly exists to support skin.reflect.
//----------------------------------------------------------------
struct Numeric {
    enum Type { U64, S64, Double };
    Type type;
    union {
        u64 repU64;
        s64 repS64;
        double repDouble;
    };

    Numeric() : type(U64), repU64(0) {
    }

    void operator=(bool v) {
        type = U64;
        repU64 = v;
    }
    void operator=(u8 v) {
        type = U64;
        repU64 = v;
    }
    void operator=(u16 v) {
        type = U64;
        repU64 = v;
    }
    void operator=(u32 v) {
        type = U64;
        repU64 = v;
    }
    void operator=(u64 v) {
        type = U64;
        repU64 = v;
    }
    void operator=(s8 v) {
        type = S64;
        repS64 = v;
    }
    void operator=(s16 v) {
        type = S64;
        repS64 = v;
    }
    void operator=(s32 v) {
        type = S64;
        repS64 = v;
    }
    void operator=(s64 v) {
        type = S64;
        repS64 = v;
    }
    void operator=(float v) {
        type = Double;
        repDouble = v;
    }
    void operator=(double v) {
        type = Double;
        repDouble = v;
    }

    template <typename T>
    T cast(bool& precise) {
        T result;
        switch (type) {
            case U64:
                result = (T) repU64;
                precise = ((u64) result == repU64);
                break;
            case S64:
                result = (T) repS64;
                precise = ((s64) result == repS64);
                break;
            case Double:
                result = (T) repDouble;
                precise = ((double) result == repDouble);
                break;
        }
        return result;
    }

    template <typename T>
    T cast() {
        bool dummy;
        return cast<T>(dummy);
    }
};

} // namespace ply
