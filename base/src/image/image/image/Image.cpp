/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <image/Core.h>
#include <image/Image.h>

namespace ply {
namespace image {

u8 Image::FormatToBPP[] = {
    0,  // Unknown
    1,  // Char
    1,  // Byte
    2,  // Byte2
    4,  // ARGB
    4,  // ABGR
    4,  // RGBA
    4,  // BGRA
    4,  // Float
    2,  // S16
    2,  // U16
    4,  // S16_2
    2,  // Half
    8,  // Float2
    4,  // Half2
    16, // Float4
    8,  // Half4
    4,  // D24S8
};

static_assert(PLY_STATIC_ARRAY_SIZE(Image::FormatToBPP) == (u32) Format::NumFormats,
              "FormatToBPP table size mismatch");

void linear_to_srgb(Image& dst, const Image& src) {
    PLY_ASSERT(dst.stride >= dst.width * 4);
    PLY_ASSERT(src.stride >= src.width);
    PLY_ASSERT(dst.width == src.width);
    PLY_ASSERT(dst.height == src.height);
    char* dst_row = dst.data;
    char* dst_row_end = dst_row + dst.stride * dst.height;
    char* src_row = src.data;
    while (dst_row < dst_row_end) {
        u32* d = (u32*) dst_row;
        u32* d_end = d + dst.width;
        char* s = src_row;
        while (d < d_end) {
            float inv_gamma = powf(*s / 255.f, 1 / 2.2f);
            u32 value = u32(roundf(inv_gamma * 255.f));
            *d = ((0x10101 * value) & 0xffffff) | ((255 - *s) << 24);
            //            *d |= 0xff000000;
            d++;
            s++;
        }
        dst_row += dst.stride;
        src_row += src.stride;
    }
}

OwnImage copy(const Image& im) {
    OwnImage dst{im.width, im.height, im.format};
    u32 bytes_per_row = Image::FormatToBPP[(u32) im.format] * im.width;
    for (s32 y = 0; y < im.height; y++) {
        memcpy(dst.get_pixel(0, y), im.get_pixel(0, y), bytes_per_row);
    }
    return dst;
}

void clear(Image& image, u32 value) {
    PLY_ASSERT(image.bytespp == 4);

    char* dst_row = image.data;
    char* dst_row_end = dst_row + image.height * image.stride;
    while (dst_row < dst_row_end) {
        u32* dst = (u32*) dst_row;
        u32* dst_end = dst + image.width;
        while (dst < dst_end) {
            *dst = value;
            dst++;
        }
        dst_row += image.stride;
    }
}

void clear(Image& image, float value) {
    PLY_ASSERT(image.is_float());

    char* dst_row = image.data;
    char* dst_row_end = dst_row + image.height * image.stride;
    while (dst_row < dst_row_end) {
        float* dst = (float*) dst_row;
        float* dst_end = dst + image.width;
        while (dst < dst_end) {
            *dst = value;
            dst++;
        }
        dst_row += image.stride;
    }
}

void clear(Image& image, const Float4& value) {
    PLY_ASSERT(image.is_float4());

    char* dst_row = image.data;
    char* dst_row_end = dst_row + image.height * image.stride;
    while (dst_row < dst_row_end) {
        Float4* dst = (Float4*) dst_row;
        Float4* dst_end = dst + image.width;
        while (dst < dst_end) {
            *dst = value;
            dst++;
        }
        dst_row += image.stride;
    }
}

void vertical_flip(Image& dst, const Image& src) {
    PLY_ASSERT(dst.format == src.format);
    PLY_ASSERT(same_dims(src, dst));
    for (s32 y = 0; y < dst.height; y++) {
        char* dst_row = dst.get_pixel(0, y);
        const char* src_row = src.get_pixel(0, dst.height - 1 - y);
        memcpy(dst_row, src_row,
               image::Image::FormatToBPP[(u32) dst.format] * dst.width);
    }
}

void copy32_bit(Image& dst, const Image& src) {
    PLY_ASSERT(dst.bytespp == 4);
    PLY_ASSERT(src.bytespp == 4);
    PLY_ASSERT(same_dims(dst, src));

    for (s32 y = 0; y < dst.height; y++) {
        u32* d = (u32*) dst.get_pixel(0, y);
        u32* d_end = d + dst.width;
        u32* s = (u32*) src.get_pixel(0, y);
        while (d < d_end) {
            *d = *s;
            d++;
            s++;
        }
    }
}

void convert_float_to_half(Image& half_im, const Image& float_im) {
    PLY_ASSERT(half_im.is_half());
    PLY_ASSERT(float_im.is_float());
    PLY_ASSERT(same_dims(half_im, float_im));

    for (s32 y = 0; y < half_im.height; y++) {
        for (s32 x = 0; x < half_im.width; x++) {
            u32 single = *(u32*) float_im.get_pixel(x, y);
            u16 zero_mask =
                -(single + single >= 0x71000000);  // is exponent is less than -14, this
                                                   // will force the result to zero
            u16 half = ((single >> 16) & 0x8000) | // sign
                       (((single >> 13) - 0x1c000) &
                        0x7fff); // exponent and mantissa (just assume exponent is
                                 // small enough to avoid wrap around)
            *(u16*) half_im.get_pixel(x, y) = half & zero_mask;
        }
    }
}

void convert_float4_to_half4(Image& half_im, const Image& float_im) {
    PLY_ASSERT(half_im.is_half4());
    PLY_ASSERT(float_im.is_float4());
    PLY_ASSERT(same_dims(half_im, float_im));

    for (s32 y = 0; y < half_im.height; y++) {
        u16* dst = (u16*) half_im.get_pixel(0, y);
        u16* dst_end = dst + half_im.width * 4;
        u32* src = (u32*) float_im.get_pixel(0, y);
        while (dst < dst_end) {
            u32 single = *src;
            u16 zero_mask =
                -(single + single >= 0x71000000);  // is exponent is less than -14, this
                                                   // will force the result to zero
            u16 half = ((single >> 16) & 0x8000) | // sign
                       (((single >> 13) - 0x1c000) &
                        0x7fff); // exponent and mantissa (just assume exponent is
                                 // small enough to avoid wrap around)
            *dst = half & zero_mask;
            dst++;
            src++;
        }
    }
}

void convert_float2_to_half2(Image& half_im, const Image& float_im) {
    PLY_ASSERT(half_im.is_half2());
    PLY_ASSERT(float_im.is_float2());
    PLY_ASSERT(same_dims(half_im, float_im));

    for (s32 y = 0; y < half_im.height; y++) {
        u16* dst = (u16*) half_im.get_pixel(0, y);
        u16* dst_end = dst + half_im.width * 2;
        u32* src = (u32*) float_im.get_pixel(0, y);
        while (dst < dst_end) {
            u32 single = *src;
            u16 zero_mask =
                -(single + single >= 0x71000000);  // is exponent is less than -14, this
                                                   // will force the result to zero
            u16 half = ((single >> 16) & 0x8000) | // sign
                       (((single >> 13) - 0x1c000) &
                        0x7fff); // exponent and mantissa (just assume exponent is
                                 // small enough to avoid wrap around)
            *dst = half & zero_mask;
            dst++;
            src++;
        }
    }
}

} // namespace image
} // namespace ply
