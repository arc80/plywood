/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
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
inline u32 to_rgba(const Int4<u8>& color) {
    return (color.w) | (color.z << 8) | (color.y << 16) | (color.x << 16);
}
inline u32 to_abgr(const Int4<u8>& color) {
    return (const u32&) color;
}
#else
inline u32 to_rgba(const Int4<u8>& color) {
    return (const u32&) color;
}
inline u32 to_abgr(const Int4<u8>& color) {
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

    void on_post_serialize() {
        PLY_ASSERT(format < Format::NumFormats);
        bytespp = FormatToBPP[(u8) format];
    }

    u32 size() const {
        PLY_ASSERT(stride > 0);
        return safe_demote<u32>(stride * height);
    }
    bool is_char() const {
        return format == Format::Char && bytespp == 1;
    }
    bool is_byte() const {
        return format == Format::Byte && bytespp == 1;
    }
    bool is_byte2() const {
        return format == Format::Byte2 && bytespp == 2;
    }
    bool is_float() const {
        return format == Format::Float && bytespp == 4;
    }
    bool is_half() const {
        return format == Format::Half && bytespp == 2;
    }
    bool is_float2() const {
        return format == Format::Float2 && bytespp == 8;
    }
    bool is_half2() const {
        return format == Format::Half2 && bytespp == 4;
    }
    bool is_float4() const {
        return format == Format::Float4 && bytespp == 16;
    }
    bool is_half4() const {
        return format == Format::Half4 && bytespp == 8;
    }
    bool is_u16() const {
        return format == Format::U16 && bytespp == 2;
    }

    friend bool same_dims(const Image& a, const Image& b) {
        return a.width == b.width && a.height == b.height;
    }

    bool is_square() const {
        return width == height;
    }

    IntVec2 dims() const {
        return {width, height};
    }

    IntRect get_rect() const {
        return {{0, 0}, {width, height}};
    }

    char* get_pixel(s32 x, s32 y) {
        PLY_ASSERT(get_rect().contains(IntVec2{x, y}));
        return data + y * stride + x * bytespp;
    }

    const char* get_pixel(s32 x, s32 y) const {
        PLY_ASSERT(get_rect().contains(IntVec2{x, y}));
        return data + y * stride + x * bytespp;
    }

    template <typename T>
    T* get(s32 x, s32 y) {
        PLY_ASSERT(sizeof(T) == FormatToBPP[(u8) format]);
        return (T*) get_pixel(x, y);
    }

    template <typename T>
    const T* get(s32 x, s32 y) const {
        PLY_ASSERT(sizeof(T) == FormatToBPP[(u8) format]);
        return (const T*) get_pixel(x, y);
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
            Heap.free(data);
        }
        PLY_ASSERT(fmt < Format::NumFormats);
        bytespp = FormatToBPP[(u8) fmt];
        width = w;
        height = h;
        stride = width * bytespp;
        format = fmt;
        data = (char*) Heap.alloc(stride * height);
    }

    ~OwnImage() {
        if (data) {
            Heap.free(data);
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
            Heap.free(data);
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
    PLY_ASSERT(im.get_rect().contains(r));
    return Image{im.data + r.mins.y * im.stride + r.mins.x * im.bytespp, im.stride,
                 r.width(), r.height(), im.format};
}

OwnImage copy(const Image& im);
void clear(Image& image, u32 value);
void clear(Image& image, float value);
void clear(Image& image, const Float4& value);
void vertical_flip(Image& dst, const Image& src);
void copy32_bit(Image& dst, const Image& src);
void linear_to_srgb(Image& dst, const Image& src);
void convert_float_to_half(Image& half_im, const Image& float_im);
void convert_float2_to_half2(Image& half_im, const Image& float_im);
void convert_float4_to_half4(Image& half_im, const Image& float_im);

} // namespace image
} // namespace ply
