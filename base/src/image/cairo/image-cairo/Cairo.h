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

    PLY_INLINE Scope(Context* cr) : cr{cr} {
        cairo_save((cairo_t*) cr);
    }
    PLY_INLINE Scope(Scope&& other) {
        this->cr = other.cr;
        other.cr = nullptr;
    }
    PLY_INLINE ~Scope() {
        if (this->cr) {
            cairo_restore((cairo_t*) this->cr);
        }
    }
};

struct Font {
    PLY_INLINE void incRef() {
        cairo_font_face_reference((cairo_font_face_t*) this);
    }
    PLY_INLINE void decRef() {
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

    static PLY_INLINE Owned<Surface> create(image::Image& image) {
        PLY_ASSERT(image.format == RequiredFormat);
        PLY_ASSERT(image.bytespp == 4);
        cairo_surface_t* surface =
            cairo_image_surface_create_for_data((unsigned char*) image.data, CAIRO_FORMAT_ARGB32,
                                                image.width, image.height, image.stride);
        PLY_ASSERT(cairo_surface_status(surface) == CAIRO_STATUS_SUCCESS);
        return (Surface*) surface;
    }
    ~Surface() = delete; // Must use Owned<>
    PLY_INLINE void destroy() {
        cairo_surface_destroy((cairo_surface_t*) this);
    }
};

struct Context {
    static PLY_INLINE Owned<Context> create(Surface* surface) {
        cairo_t* cr = cairo_create((cairo_surface_t*) surface);
        PLY_ASSERT(cairo_status(cr) == CAIRO_STATUS_SUCCESS);
        return (Context*) cr;
    }
    ~Context() = delete; // Must use Owned<>
    PLY_INLINE void destroy() {
        cairo_destroy((cairo_t*) this);
    }
    PLY_INLINE void setSourceRGB(const Float3& c) {
        cairo_set_source_rgb((cairo_t*) this, c.r(), c.g(), c.b());
    }
    PLY_INLINE void setSourceRGBA(const Float4& c) {
        cairo_set_source_rgba((cairo_t*) this, c.r(), c.g(), c.b(), c.a());
    }
    PLY_INLINE void setLineWidth(float w) {
        cairo_set_line_width((cairo_t*) this, w);
    }
    PLY_INLINE void scale(const Float2& s) {
        cairo_scale((cairo_t*) this, s.x, s.y);
    }
    PLY_INLINE void newSubPath() {
        cairo_new_sub_path((cairo_t*) this);
    }
    PLY_INLINE Float2 getCurrentPoint() const {
        double x, y;
        cairo_get_current_point((cairo_t*) this, &x, &y);
        return {(float) x, (float) y};
    }
    PLY_INLINE void moveTo(const Float2& p) {
        cairo_move_to((cairo_t*) this, p.x, p.y);
    }
    PLY_INLINE void lineTo(const Float2& p) {
        cairo_line_to((cairo_t*) this, p.x, p.y);
    }
    PLY_INLINE void relLineTo(const Float2& p) {
        cairo_rel_line_to((cairo_t*) this, p.x, p.y);
    }
    PLY_INLINE void curveTo(const Float2& p1, const Float2& p2, const Float2& p3) {
        cairo_curve_to((cairo_t*) this, p1.x, p1.y, p2.x, p2.y, p3.x, p3.y);
    }
    PLY_INLINE void closePath() {
        cairo_close_path((cairo_t*) this);
    }
    PLY_INLINE void arc(const Float2& p, float radius, float startRad, float endRad) {
        cairo_arc((cairo_t*) this, p.x, p.y, radius, startRad, endRad);
    }
    PLY_INLINE void arcNegative(const Float2& p, float radius, float startRad, float endRad) {
        cairo_arc_negative((cairo_t*) this, p.x, p.y, radius, startRad, endRad);
    }
    PLY_INLINE void circle(const Float2& p, float radius) {
        cairo_arc((cairo_t*) this, p.x, p.y, radius, 0, 2 * Pi);
    }
    PLY_INLINE void rectangle(const Rect& r) {
        cairo_rectangle((cairo_t*) this, r.mins.x, r.mins.y, r.width(), r.height());
    }
    PLY_INLINE void showText(const char* utf8) {
        cairo_show_text((cairo_t*) this, utf8);
    }
    PLY_INLINE void paint() {
        cairo_paint((cairo_t*) this);
    }
    PLY_INLINE void fill() {
        cairo_fill((cairo_t*) this);
    }
    PLY_INLINE void fillPreserve() {
        cairo_fill_preserve((cairo_t*) this);
    }
    PLY_INLINE void stroke() {
        cairo_stroke((cairo_t*) this);
    }
    PLY_INLINE void setDash(std::initializer_list<double> dashes, double offset = 0.0) {
        cairo_set_dash((cairo_t*) this, dashes.begin(), safeDemote<int>(dashes.size()), offset);
    }
    PLY_INLINE void setLineJoin(cairo_line_join_t join) {
        cairo_set_line_join((cairo_t*) this, join);
    }
    PLY_INLINE Font* getFontFace() const {
        return (Font*) cairo_get_font_face((cairo_t*) this);
    }
    PLY_INLINE void setFontFace(Font* font) {
        cairo_set_font_face((cairo_t*) this, (cairo_font_face_t*) font);
    }
    PLY_INLINE void setFontSize(float size) {
        cairo_set_font_size((cairo_t*) this, size);
    }
    PLY_INLINE cairo_text_extents_t textExtents(const StringView& s) {
        cairo_text_extents_t extents;
        cairo_text_extents((cairo_t*) this, s.withNullTerminator().bytes, &extents);
        return extents;
    }
    PLY_INLINE void showText(StringView s) {
        cairo_show_text((cairo_t*) this, s.withNullTerminator().bytes);
    }
    PLY_INLINE cairo_font_extents_t fontExtents() {
        cairo_font_extents_t extents;
        cairo_font_extents((cairo_t*) this, &extents);
        return extents;
    }
    PLY_INLINE void translate(const Float2& t) {
        cairo_translate((cairo_t*) this, t.x, t.y);
    }
    PLY_INLINE void rotate(float angle) {
        cairo_rotate((cairo_t*) this, angle);
    }
    PLY_INLINE void clip() {
        cairo_clip((cairo_t*) this);
    }
    PLY_INLINE Scope save() {
        return {this};
    }
    PLY_INLINE cairo_t* toContext() {
        return (cairo_t*) this;
    }
};

} // namespace cairo
} // namespace ply
