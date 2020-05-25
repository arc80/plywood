/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <image/Core.h>
#include <image-png/PNG.h>
#include <png.h>

namespace ply {
namespace image {

OwnImage readPNG(ply::InStream* in) {
    return {0, 0, Format::Unknown};
}

void writePNG(Image& im, ply::OutStream* out) {
    // Create png_ptr
    png_structp png_ptr = png_create_write_struct(
        PNG_LIBPNG_VER_STRING, nullptr,
        // error_fn
        [](png_structp, png_const_charp) {
            // FIXME: Do something
            PLY_FORCE_CRASH();
        },
        // warn_fn
        [](png_structp, png_const_charp) {
            // FIXME: Do something
            PLY_FORCE_CRASH();
        });
    PLY_ASSERT(png_ptr);

    // Create info_ptr
    png_infop info_ptr = png_create_info_struct(png_ptr);
    PLY_ASSERT(info_ptr);

    // FIXME: user_error_fn should do a longjmp to here. See png_safe_execute (pngerror.c) for
    // example.
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        PLY_FORCE_CRASH();
        return;
    }

    // Set I/O callbacks
    png_set_write_fn(
        png_ptr, out,
        [](png_structp png_ptr, png_bytep buf, png_size_t size) {
            ply::OutStream* out = (ply::OutStream*) png_get_io_ptr(png_ptr);
            out->write({buf, safeDemote<u32>(size)});
        },
        [](png_structp png_ptr) {
            ply::OutStream* out = (ply::OutStream*) png_get_io_ptr(png_ptr);
            out->flushMem();
        });

    // Write header
    int pngColorType = PNG_COLOR_TYPE_RGB_ALPHA;
    switch (im.format) {
        case image::Format::BGRA:
        case image::Format::RGBA:
            PLY_ASSERT(im.bytespp == 4);
            break;
        case image::Format::Byte:
            pngColorType = PNG_COLOR_TYPE_GRAY;
            PLY_ASSERT(im.bytespp == 1);
            break;
        default:
            PLY_ASSERT(0); // Unsupported format
            break;
    }
    png_set_IHDR(png_ptr, info_ptr, im.width, im.height, 8, pngColorType, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_set_sRGB(png_ptr, info_ptr, PNG_sRGB_INTENT_PERCEPTUAL);
    png_write_info(png_ptr, info_ptr);
    switch (im.format) {
        case image::Format::BGRA:
            png_set_bgr(png_ptr);
            break;
        case image::Format::RGBA:
        case image::Format::Byte:
            break;
        default:
            PLY_ASSERT(0); // Unsupported format
            break;
    }

    // Write data
    for (s32 y = 0; y < im.height; y++) {
        png_write_row(png_ptr, im.getPixel(0, y));
    }
    png_write_end(png_ptr, info_ptr);

    // Cleanup
    png_destroy_write_struct(&png_ptr, &info_ptr);
}

} // namespace image
} // namespace ply
