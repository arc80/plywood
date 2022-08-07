/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <image/Core.h>

namespace ply {
namespace image {

enum class Format : u8 {
    Unknown = 0,
    Char,
    Byte,
    Byte2,
    RGBA,
    BGRA,
    ARGB,
    ABGR,
    Float,
    S16,
    U16,
    S16_2,
    Half,
    Float2,
    Half2,
    Float4,
    Half4,
    D24S8,
    // Note: make sure to update OfflineTexture.cpp (for now) if more formats are added
    NumFormats
};

#if PLY_IS_BIG_ENDIAN
inline u32 toRGBA(const Int4<u8>& color) {
    return (color.w) | (color.z << 8) | (color.y << 16) | (color.x << 16);
}
inline u32 toABGR(const Int4<u8>& color) {
    return (const u32&) color;
}
#else
inline u32 toRGBA(const Int4<u8>& color) {
    return (const u32&) color;
}
inline u32 toABGR(const Int4<u8>& color) {
    return (color.w) | (color.z << 8) | (color.y << 16) | (color.x << 16);
}
#endif

struct Image {
    static u8 FormatToBPP[];

    char* data = nullptr;
    s32 stride = 0;
    s32 width = 0;
    s32 height = 0;
    u8 bytespp = 0;
    Format format = Format::Unknown;

    Image() = default;
    Image(char* data, s32 stride, s32 w, s32 h, Format fmt)
        : data{data}, stride{stride}, width{w}, height{h}, format{fmt} {
        PLY_ASSERT(fmt < Format::NumFormats);
        bytespp = FormatToBPP[(u8) fmt];
    }
    // void operator=(const Image& im) = delete;

    void onPostSerialize() {
        PLY_ASSERT(format < Format::NumFormats);
        bytespp = FormatToBPP[(u8) format];
    }

    u32 size() const {
        PLY_ASSERT(stride > 0);
        return safeDemote<u32>(stride * height);
    }
    bool isChar() const {
        return format == Format::Char && bytespp == 1;
    }
    bool isByte() const {
        return format == Format::Byte && bytespp == 1;
    }
    bool isByte2() const {
        return format == Format::Byte2 && bytespp == 2;
    }
    bool isFloat() const {
        return format == Format::Float && bytespp == 4;
    }
    bool isHalf() const {
        return format == Format::Half && bytespp == 2;
    }
    bool isFloat2() const {
        return format == Format::Float2 && bytespp == 8;
    }
    bool isHalf2() const {
        return format == Format::Half2 && bytespp == 4;
    }
    bool isFloat4() const {
        return format == Format::Float4 && bytespp == 16;
    }
    bool isHalf4() const {
        return format == Format::Half4 && bytespp == 8;
    }
    bool isU16() const {
        return format == Format::U16 && bytespp == 2;
    }

    friend bool sameDims(const Image& a, const Image& b) {
        return a.width == b.width && a.height == b.height;
    }

    bool isSquare() const {
        return width == height;
    }

    IntVec2 dims() const {
        return {width, height};
    }

    IntRect getRect() const {
        return {{0, 0}, {width, height}};
    }

    char* getPixel(s32 x, s32 y) {
        PLY_ASSERT(getRect().contains(IntVec2{x, y}));
        return data + y * stride + x * bytespp;
    }

    const char* getPixel(s32 x, s32 y) const {
        PLY_ASSERT(getRect().contains(IntVec2{x, y}));
        return data + y * stride + x * bytespp;
    }

    template <typename T>
    T* get(s32 x, s32 y) {
        PLY_ASSERT(sizeof(T) == FormatToBPP[(u8) format]);
        return (T*) getPixel(x, y);
    }

    template <typename T>
    const T* get(s32 x, s32 y) const {
        PLY_ASSERT(sizeof(T) == FormatToBPP[(u8) format]);
        return (const T*) getPixel(x, y);
    }
};

struct OwnImage : Image {
    OwnImage() = default;

    OwnImage(s32 w, s32 h, Format fmt) {
        data = nullptr;
        alloc(w, h, fmt);
    }

    void alloc(s32 w, s32 h, Format fmt) {
        if (data) {
            PLY_HEAP.free(data);
        }
        PLY_ASSERT(fmt < Format::NumFormats);
        bytespp = FormatToBPP[(u8) fmt];
        width = w;
        height = h;
        stride = width * bytespp;
        format = fmt;
        data = (char*) PLY_HEAP.alloc(stride * height);
    }

    ~OwnImage() {
        if (data) {
            PLY_HEAP.free(data);
        }
    }

    OwnImage(OwnImage&& other) {
        data = other.data;
        stride = other.stride;
        width = other.width;
        height = other.height;
        bytespp = other.bytespp;
        format = other.format;
        other.release();
    }

    char* release() {
        char* result = data;
        data = nullptr;
        stride = 0;
        width = 0;
        height = 0;
        bytespp = 0;
        format = Format::Unknown;
        return result;
    }

    void operator=(OwnImage&& other) {
        if (data) {
            PLY_HEAP.free(data);
        }
        data = other.data;
        stride = other.stride;
        width = other.width;
        height = other.height;
        bytespp = other.bytespp;
        format = other.format;
        other.data = nullptr;
        other.stride = 0;
        other.width = 0;
        other.height = 0;
        other.bytespp = 0;
        other.format = Format::Unknown;
    }
};

inline Image crop(const Image& im, const IntRect& r) {
    PLY_ASSERT(im.getRect().contains(r));
    return Image{im.data + r.mins.y * im.stride + r.mins.x * im.bytespp, im.stride, r.width(),
                 r.height(), im.format};
}

OwnImage copy(const Image& im);
void clear(Image& image, u32 value);
void clear(Image& image, float value);
void clear(Image& image, const Float4& value);
void verticalFlip(Image& dst, const Image& src);
void copy32Bit(Image& dst, const Image& src);
void linearToSRGB(Image& dst, const Image& src);
void convertFloatToHalf(Image& halfIm, const Image& floatIm);
void convertFloat2ToHalf2(Image& halfIm, const Image& floatIm);
void convertFloat4ToHalf4(Image& halfIm, const Image& floatIm);

} // namespace image
} // namespace ply
