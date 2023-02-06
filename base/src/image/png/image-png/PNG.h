/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <image/Image.h>
#include <ply-runtime/io/InStream.h>
#include <ply-runtime/io/OutStream.h>

namespace ply {
namespace image {

OwnImage read_png(ply::InStream* in);
void write_png(const Image& im, ply::OutStream* out);

} // namespace image
} // namespace ply
