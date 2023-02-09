/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-codec/Muxer.h>
#include <image-cairo/Cairo.h>

using namespace ply;
static constexpr u32 NumPoints = 1000;

int main() {
    Owned<OutPipe> out = FileSystem.open_pipe_for_write("vector-animation.mp4");
    VideoOptions video_opts{240, 240};
    Owned<Muxer> muxer = create_muxer(out, &video_opts, nullptr, "mp4");
    for (u32 j = 0; j < 120; j++) {
        image::Image image;
        muxer->begin_video_frame(image);
        {
            Owned<cairo::Surface> surface = cairo::Surface::create(image);
            Owned<cairo::Context> cr = cairo::Context::create(surface);
            cr->set_source_rgb({1, 1, 1});
            cr->paint();
            const float num_loops = mix(4.f, 22.f, j / 120.f);
            for (u32 i = 0; i <= NumPoints; i++) {
                vec2 c1 = Complex::from_angle(2 * Pi * i * num_loops / NumPoints);
                vec2 c2 = Complex::from_angle(2 * Pi * i / NumPoints);
                cr->line_to(vec2{120, 120} + c1 * 68 + c2 * 48);
            }
            cr->set_line_width(1.5f);
            cr->set_source_rgb({0.2f, 0.5f, 0.5f});
            cr->stroke();
        }
        muxer->end_video_frame();
    }
    return 0;
}
