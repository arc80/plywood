/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/time/CPUTimer.h>

namespace ply {
namespace fmt {

//----------------------------------------------------
// fmt::WithRadix, fmt::Hex
//----------------------------------------------------
struct WithRadix {
    enum Type {
        U64 = 0,
        S64,
        Double,
    };
    union {
        double double_;
        u64 u64_;
        s64 s64_;
    };
    u32 type : 2;
    u32 capitalize : 1;
    u32 radix : 29;

    template <typename SrcType,
              typename std::enable_if_t<std::is_floating_point<SrcType>::value, int> = 0>
    inline WithRadix(SrcType v, u32 radix, bool capitalize = false) {
        this->double_ = v;
        this->type = U64;
        this->capitalize = capitalize ? 1 : 0;
        this->radix = radix;
    }
    template <typename SrcType,
              typename std::enable_if_t<std::is_unsigned<SrcType>::value, int> = 0>
    inline WithRadix(SrcType v, u32 radix, bool capitalize = false) {
        this->u64_ = v;
        this->type = U64;
        this->capitalize = capitalize ? 1 : 0;
        this->radix = radix;
    }
    template <typename SrcType, typename std::enable_if_t<std::is_signed<SrcType>::value, int> = 0>
    inline WithRadix(SrcType v, u32 radix, bool capitalize = false) {
        this->s64_ = v;
        this->type = S64;
        this->capitalize = capitalize ? 1 : 0;
        this->radix = radix;
    }
};

struct Hex : WithRadix {
    template <typename SrcType,
              typename std::enable_if_t<std::is_arithmetic<SrcType>::value, int> = 0>
    inline Hex(SrcType v, bool capitalize = false) : WithRadix{v, 16, capitalize} {
    }
};

//-----------------------------------------------------------
// fmt::EscapedString
//-----------------------------------------------------------
struct EscapedString {
    StringView view;
    u32 maxPoints = 0;
    PLY_INLINE EscapedString(StringView view, u32 maxPoints = 0)
        : view{view}, maxPoints{maxPoints} {
    }
};

//-----------------------------------------------------------
// fmt::XMLEscape
//-----------------------------------------------------------
struct XMLEscape {
    StringView view;
    u32 maxPoints = 0;
    PLY_INLINE XMLEscape(StringView view, u32 maxPoints = 0) : view{view}, maxPoints{maxPoints} {
    }
};

//-----------------------------------------------------------
// fmt::CmdLineArg_WinCrt
//-----------------------------------------------------------
struct CmdLineArg_WinCrt {
    StringView view;
    PLY_INLINE CmdLineArg_WinCrt(StringView view) : view{view} {
    }
};

//----------------------------------------------------
// TypePrinters
//----------------------------------------------------
template <typename DstType, typename SrcType>
struct TypePrinter_Cast {
    static PLY_INLINE void print(OutStream* outs, const SrcType& value) {
        TypePrinter<DstType>::print(outs, value);
    }
};

template <>
struct TypePrinter<u32> : TypePrinter_Cast<u64, u32> {};
template <>
struct TypePrinter<u16> : TypePrinter_Cast<u64, u16> {};
template <>
struct TypePrinter<u8> : TypePrinter_Cast<u64, u8> {};
template <>
struct TypePrinter<s32> : TypePrinter_Cast<s64, s32> {};
template <>
struct TypePrinter<s16> : TypePrinter_Cast<s64, s16> {};
template <>
struct TypePrinter<s8> : TypePrinter_Cast<s64, s8> {};
template <>
struct TypePrinter<float> : TypePrinter_Cast<double, float> {};
template <>
struct TypePrinter<String> : TypePrinter_Cast<StringView, String> {};
template <>
struct TypePrinter<HybridString> : TypePrinter_Cast<StringView, HybridString> {};
template <>
struct TypePrinter<char> {
    static PLY_INLINE void print(OutStream* outs, char c) {
        outs->writeByte(c);
    }
};
template <>
struct TypePrinter<StringView> {
    static PLY_DLL_ENTRY void print(OutStream* outs, StringView value);
};
template <int N>
struct TypePrinter<char[N]> {
    static PLY_INLINE void print(OutStream* outs, const char* c) {
        PLY_ASSERT(c[N - 1] == 0); // must be a null-terminated string literal
        TypePrinter<StringView>::print(outs, {c, N - 1});
    }
};
template <>
struct TypePrinter<char*> {
    static PLY_INLINE void print(OutStream* outs, const char* c) {
        TypePrinter<StringView>::print(outs, c);
    }
};
template <>
struct TypePrinter<const char*> {
    static PLY_INLINE void print(OutStream* outs, const char* c) {
        TypePrinter<StringView>::print(outs, c);
    }
};

template <>
struct TypePrinter<WithRadix> {
    static PLY_DLL_ENTRY void print(OutStream* outs, const WithRadix& value);
};
template <>
struct TypePrinter<Hex> {
    static PLY_INLINE void print(OutStream* outs, const Hex& value) {
        TypePrinter<WithRadix>::print(outs, value);
    }
};
template <>
struct TypePrinter<u64> {
    static PLY_DLL_ENTRY void print(OutStream* outs, u64 value);
};
template <>
struct TypePrinter<s64> {
    static PLY_DLL_ENTRY void print(OutStream* outs, s64 value);
};
template <>
struct TypePrinter<double> {
    static PLY_DLL_ENTRY void print(OutStream* outs, double value);
};
template <>
struct TypePrinter<bool> {
    static PLY_DLL_ENTRY void print(OutStream* outs, bool value);
};
template <>
struct TypePrinter<CPUTimer::Duration> {
    static PLY_DLL_ENTRY void print(OutStream* outs, CPUTimer::Duration value);
};
template <>
struct TypePrinter<EscapedString> {
    static PLY_DLL_ENTRY void print(OutStream* outs, const EscapedString& value);
};
template <>
struct TypePrinter<XMLEscape> {
    static PLY_DLL_ENTRY void print(OutStream* outs, const XMLEscape& value);
};
template <>
struct TypePrinter<CmdLineArg_WinCrt> {
    static PLY_DLL_ENTRY void print(OutStream* outs, const CmdLineArg_WinCrt& value);
};

} // namespace fmt
} // namespace ply
