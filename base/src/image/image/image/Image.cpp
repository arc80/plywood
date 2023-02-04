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

void linearToSRGB(Image& dst, const Image& src) {
    PLY_ASSERT(dst.stride >= dst.width * 4);
    PLY_ASSERT(src.stride >= src.width);
    PLY_ASSERT(dst.width == src.width);
    PLY_ASSERT(dst.height == src.height);
    char* dstRow = dst.data;
    char* dstRowEnd = dstRow + dst.stride * dst.height;
    char* srcRow = src.data;
    while (dstRow < dstRowEnd) {
        u32* d = (u32*) dstRow;
        u32* dEnd = d + dst.width;
        char* s = srcRow;
        while (d < dEnd) {
            float invGamma = powf(*s / 255.f, 1 / 2.2f);
            u32 value = u32(roundf(invGamma * 255.f));
            *d = ((0x10101 * value) & 0xffffff) | ((255 - *s) << 24);
            //            *d |= 0xff000000;
            d++;
            s++;
        }
        dstRow += dst.stride;
        srcRow += src.stride;
    }
}

OwnImage copy(const Image& im) {
    OwnImage dst{im.width, im.height, im.format};
    u32 bytesPerRow = Image::FormatToBPP[(u32) im.format] * im.width;
    for (s32 y = 0; y < im.height; y++) {
        memcpy(dst.getPixel(0, y), im.getPixel(0, y), bytesPerRow);
    }
    return dst;
}

void clear(Image& image, u32 value) {
    PLY_ASSERT(image.bytespp == 4);

    char* dstRow = image.data;
    char* dstRowEnd = dstRow + image.height * image.stride;
    while (dstRow < dstRowEnd) {
        u32* dst = (u32*) dstRow;
        u32* dstEnd = dst + image.width;
        while (dst < dstEnd) {
            *dst = value;
            dst++;
        }
        dstRow += image.stride;
    }
}

void clear(Image& image, float value) {
    PLY_ASSERT(image.isFloat());

    char* dstRow = image.data;
    char* dstRowEnd = dstRow + image.height * image.stride;
    while (dstRow < dstRowEnd) {
        float* dst = (float*) dstRow;
        float* dstEnd = dst + image.width;
        while (dst < dstEnd) {
            *dst = value;
            dst++;
        }
        dstRow += image.stride;
    }
}

void clear(Image& image, const Float4& value) {
    PLY_ASSERT(image.isFloat4());

    char* dstRow = image.data;
    char* dstRowEnd = dstRow + image.height * image.stride;
    while (dstRow < dstRowEnd) {
        Float4* dst = (Float4*) dstRow;
        Float4* dstEnd = dst + image.width;
        while (dst < dstEnd) {
            *dst = value;
            dst++;
        }
        dstRow += image.stride;
    }
}

void verticalFlip(Image& dst, const Image& src) {
    PLY_ASSERT(dst.format == src.format);
    PLY_ASSERT(sameDims(src, dst));
    for (s32 y = 0; y < dst.height; y++) {
        char* dstRow = dst.getPixel(0, y);
        const char* srcRow = src.getPixel(0, dst.height - 1 - y);
        memcpy(dstRow, srcRow, image::Image::FormatToBPP[(u32) dst.format] * dst.width);
    }
}

void copy32Bit(Image& dst, const Image& src) {
    PLY_ASSERT(dst.bytespp == 4);
    PLY_ASSERT(src.bytespp == 4);
    PLY_ASSERT(sameDims(dst, src));

    for (s32 y = 0; y < dst.height; y++) {
        u32* d = (u32*) dst.getPixel(0, y);
        u32* dEnd = d + dst.width;
        u32* s = (u32*) src.getPixel(0, y);
        while (d < dEnd) {
            *d = *s;
            d++;
            s++;
        }
    }
}

void convertFloatToHalf(Image& halfIm, const Image& floatIm) {
    PLY_ASSERT(halfIm.isHalf());
    PLY_ASSERT(floatIm.isFloat());
    PLY_ASSERT(sameDims(halfIm, floatIm));

    for (s32 y = 0; y < halfIm.height; y++) {
        for (s32 x = 0; x < halfIm.width; x++) {
            u32 single = *(u32*) floatIm.getPixel(x, y);
            u16 zeroMask = -(single + single >= 0x71000000); // is exponent is less than -14, this
                                                             // will force the result to zero
            u16 half = ((single >> 16) & 0x8000) |           // sign
                       (((single >> 13) - 0x1c000) &
                        0x7fff); // exponent and mantissa (just assume exponent is
                                 // small enough to avoid wrap around)
            *(u16*) halfIm.getPixel(x, y) = half & zeroMask;
        }
    }
}

void convertFloat4ToHalf4(Image& halfIm, const Image& floatIm) {
    PLY_ASSERT(halfIm.isHalf4());
    PLY_ASSERT(floatIm.isFloat4());
    PLY_ASSERT(sameDims(halfIm, floatIm));

    for (s32 y = 0; y < halfIm.height; y++) {
        u16* dst = (u16*) halfIm.getPixel(0, y);
        u16* dstEnd = dst + halfIm.width * 4;
        u32* src = (u32*) floatIm.getPixel(0, y);
        while (dst < dstEnd) {
            u32 single = *src;
            u16 zeroMask = -(single + single >= 0x71000000); // is exponent is less than -14, this
                                                             // will force the result to zero
            u16 half = ((single >> 16) & 0x8000) |           // sign
                       (((single >> 13) - 0x1c000) &
                        0x7fff); // exponent and mantissa (just assume exponent is
                                 // small enough to avoid wrap around)
            *dst = half & zeroMask;
            dst++;
            src++;
        }
    }
}

void convertFloat2ToHalf2(Image& halfIm, const Image& floatIm) {
    PLY_ASSERT(halfIm.isHalf2());
    PLY_ASSERT(floatIm.isFloat2());
    PLY_ASSERT(sameDims(halfIm, floatIm));

    for (s32 y = 0; y < halfIm.height; y++) {
        u16* dst = (u16*) halfIm.getPixel(0, y);
        u16* dstEnd = dst + halfIm.width * 2;
        u32* src = (u32*) floatIm.getPixel(0, y);
        while (dst < dstEnd) {
            u32 single = *src;
            u16 zeroMask = -(single + single >= 0x71000000); // is exponent is less than -14, this
                                                             // will force the result to zero
            u16 half = ((single >> 16) & 0x8000) |           // sign
                       (((single >> 13) - 0x1c000) &
                        0x7fff); // exponent and mantissa (just assume exponent is
                                 // small enough to avoid wrap around)
            *dst = half & zeroMask;
            dst++;
            src++;
        }
    }
}

} // namespace image
} // namespace ply
