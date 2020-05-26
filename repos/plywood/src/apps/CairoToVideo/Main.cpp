/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-codec/Muxer.h>
#include <image-cairo/Cairo.h>

using namespace ply;
static constexpr u32 NumPoints = 1000;

int main() {
    Owned<OutPipe> out = FileSystem::native()->openPipeForWrite("vector-animation.mp4");
    VideoOptions videoOpts{240, 240};
    Owned<Muxer> muxer = createMuxer(out, &videoOpts, nullptr, "mp4");
    for (u32 j = 0; j < 120; j++) {
        image::Image image;
        muxer->beginVideoFrame(image);
        {
            Owned<cairo::Surface> surface = cairo::Surface::create(image);
            Owned<cairo::Context> cr = cairo::Context::create(surface);
            cr->setSourceRGB({1, 1, 1});
            cr->paint();
            const float numLoops = mix(4.f, 22.f, j / 120.f);
            for (u32 i = 0; i <= NumPoints; i++) {
                Float2 c1 = Complex::fromAngle(2 * Pi * i * numLoops / NumPoints);
                Float2 c2 = Complex::fromAngle(2 * Pi * i / NumPoints);
                cr->lineTo(Float2{120, 120} + c1 * 68 + c2 * 48);
            }
            cr->setLineWidth(1.5f);
            cr->setSourceRGB({0.2f, 0.5f, 0.5f});
            cr->stroke();
        }
        muxer->endVideoFrame();
    }
    return 0;
}
