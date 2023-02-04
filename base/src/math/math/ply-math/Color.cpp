/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-math/Core.h>
#include <ply-math/Color.h>

namespace ply {

void convertFromHex(float* values, size_t numValues, const char* hex) {
    for (size_t i = 0; i < numValues; i++) {
        int c = 0;
        for (int j = 0; j < 2; j++) {
            c <<= 4;
            if (*hex >= '0' && *hex <= '9') {
                c += *hex - '0';
            } else if (*hex >= 'a' && *hex <= 'f') {
                c += *hex - 'a' + 10;
            } else if (*hex >= 'A' && *hex <= 'F') {
                c += *hex - 'A' + 10;
            } else {
                PLY_ASSERT(0);
            }
            hex++;
        }
        values[i] = c * (1 / 255.f);
    }
    PLY_ASSERT(*hex == 0);
}

} // namespace ply
