/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <image/Image.h>
#include <ply-runtime/io/InStream.h>
#include <ply-runtime/io/OutStream.h>

namespace ply {
namespace image {

OwnImage readPNG(ply::InStream* in);
void writePNG(Image& im, ply::OutStream* out);

} // namespace image
} // namespace ply
