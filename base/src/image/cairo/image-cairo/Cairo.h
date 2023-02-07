/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <image/Image.h>
#include <ply-runtime/string/String.h>
#include <cairo.h>

namespace ply {
namespace cairo {

struct Context;

struct Scope {
    Context* cr = nullptr;

    Scope(Context* cr) : cr{cr} {
        cairo_save((cairo_t*) cr);
    }
    Scope(Scope&& other) {
        this->cr = other.cr;
        other.cr = nullptr;
    }
    ~Scope() {
        if (this->cr) {
            cairo_restore((cairo_t*) this->cr);
        }
    }
};

struct Font {
    void inc_ref() {
        cairo_font_face_reference((cairo_font_face_t*) this);
    }
    void dec_ref() {
        cairo_font_face_destroy((cairo_font_face_t*) this);
    }
    ~Font() = delete; // Must use Reference<Font>
};

struct Surface {
#if PLY_IS_BIG_ENDIAN
    static const image::Format RequiredFormat = image::Format::ARGB;
#else
    static const image::Format RequiredFormat = image::Format::BGRA;
#endif

    static Owned<Surface> create(image::Image& image) {
        PLY_ASSERT(image.format == RequiredFormat);
        PLY_ASSERT(image.bytespp == 4);
        cairo_surface_t* surface = cairo_image_surface_create_for_data(
            (unsigned char*) image.data, CAIRO_FORMAT_ARGB32, image.width, image.height,
            image.stride);
        PLY_ASSERT(cairo_surface_status(surface) == CAIRO_STATUS_SUCCESS);
        return (Surface*) surface;
    }
    ~Surface() = delete; // Must use Owned<>
    void destroy() {
        cairo_surface_destroy((cairo_surface_t*) this);
    }
};

struct Context {
    static Owned<Context> create(Surface* surface) {
        cairo_t* cr = cairo_create((cairo_surface_t*) surface);
        PLY_ASSERT(cairo_status(cr) == CAIRO_STATUS_SUCCESS);
        return (Context*) cr;
    }
    ~Context() = delete; // Must use Owned<>
    void destroy() {
        cairo_destroy((cairo_t*) this);
    }
    void set_source_rgb(const Float3& c) {
        cairo_set_source_rgb((cairo_t*) this, c.r(), c.g(), c.b());
    }
    void set_source_rgba(const Float4& c) {
        cairo_set_source_rgba((cairo_t*) this, c.r(), c.g(), c.b(), c.a());
    }
    void set_line_width(float w) {
        cairo_set_line_width((cairo_t*) this, w);
    }
    void scale(const Float2& s) {
        cairo_scale((cairo_t*) this, s.x, s.y);
    }
    void new_sub_path() {
        cairo_new_sub_path((cairo_t*) this);
    }
    Float2 get_current_point() const {
        double x, y;
        cairo_get_current_point((cairo_t*) this, &x, &y);
        return {(float) x, (float) y};
    }
    void move_to(const Float2& p) {
        cairo_move_to((cairo_t*) this, p.x, p.y);
    }
    void line_to(const Float2& p) {
        cairo_line_to((cairo_t*) this, p.x, p.y);
    }
    void rel_line_to(const Float2& p) {
        cairo_rel_line_to((cairo_t*) this, p.x, p.y);
    }
    void curve_to(const Float2& p1, const Float2& p2, const Float2& p3) {
        cairo_curve_to((cairo_t*) this, p1.x, p1.y, p2.x, p2.y, p3.x, p3.y);
    }
    void close_path() {
        cairo_close_path((cairo_t*) this);
    }
    void arc(const Float2& p, float radius, float start_rad, float end_rad) {
        cairo_arc((cairo_t*) this, p.x, p.y, radius, start_rad, end_rad);
    }
    void arc_negative(const Float2& p, float radius, float start_rad, float end_rad) {
        cairo_arc_negative((cairo_t*) this, p.x, p.y, radius, start_rad, end_rad);
    }
    void circle(const Float2& p, float radius) {
        cairo_arc((cairo_t*) this, p.x, p.y, radius, 0, 2 * Pi);
    }
    void rectangle(const Rect& r) {
        cairo_rectangle((cairo_t*) this, r.mins.x, r.mins.y, r.width(), r.height());
    }
    void show_text(const char* utf8) {
        cairo_show_text((cairo_t*) this, utf8);
    }
    void paint() {
        cairo_paint((cairo_t*) this);
    }
    void fill() {
        cairo_fill((cairo_t*) this);
    }
    void fill_preserve() {
        cairo_fill_preserve((cairo_t*) this);
    }
    void stroke() {
        cairo_stroke((cairo_t*) this);
    }
    void set_dash(std::initializer_list<double> dashes, double offset = 0.0) {
        cairo_set_dash((cairo_t*) this, dashes.begin(), check_cast<int>(dashes.size()),
                       offset);
    }
    void set_line_join(cairo_line_join_t join) {
        cairo_set_line_join((cairo_t*) this, join);
    }
    Font* get_font_face() const {
        return (Font*) cairo_get_font_face((cairo_t*) this);
    }
    void set_font_face(Font* font) {
        cairo_set_font_face((cairo_t*) this, (cairo_font_face_t*) font);
    }
    void set_font_size(float size) {
        cairo_set_font_size((cairo_t*) this, size);
    }
    cairo_text_extents_t text_extents(const StringView& s) {
        cairo_text_extents_t extents;
        cairo_text_extents((cairo_t*) this, s.with_null_terminator().bytes, &extents);
        return extents;
    }
    void show_text(StringView s) {
        cairo_show_text((cairo_t*) this, s.with_null_terminator().bytes);
    }
    cairo_font_extents_t font_extents() {
        cairo_font_extents_t extents;
        cairo_font_extents((cairo_t*) this, &extents);
        return extents;
    }
    void translate(const Float2& t) {
        cairo_translate((cairo_t*) this, t.x, t.y);
    }
    void rotate(float angle) {
        cairo_rotate((cairo_t*) this, angle);
    }
    void clip() {
        cairo_clip((cairo_t*) this);
    }
    Scope save() {
        return {this};
    }
    cairo_t* to_context() {
        return (cairo_t*) this;
    }
};

} // namespace cairo
} // namespace ply
