/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-math/Core.h>

namespace ply {

template <class V>
struct Box {
    using T = decltype(V::x);

    V mins;
    V maxs;

    // Constructors
    Box() = default; // uninitialized

    Box(const V& arg) : mins{arg}, maxs{arg} {
    }

    Box(const V& mins, const V& maxs) : mins{mins}, maxs{maxs} {
    }

    T length2() const {
        return mins.length2() + maxs.length2();
    }

    static Box<V> from_size(T minx, T miny, T width, T height) {
        return {{minx, miny}, {T(minx + width), T(miny + height)}};
    }

    static Box<V> from_size(const V& mins, const V& size) {
        return {mins, mins + size};
    }

    template <typename OtherBox>
    OtherBox to() const {
        using V2 = decltype(OtherBox::mins);
        return OtherBox{mins.template to<V2>(), maxs.template to<V2>()};
    }

    // +
    Box operator+(const Box& arg) const {
        return Box(mins + arg.mins, maxs + arg.maxs);
    }

    void operator+=(const Box& arg) {
        mins += arg.mins;
        maxs += arg.maxs;
    }

    // -
    Box operator-(const Box& arg) const {
        return Box(mins - arg.mins, maxs - arg.maxs);
    }

    void operator-=(const Box& arg) {
        mins -= arg.mins;
        maxs -= arg.maxs;
    }

    // *
    Box operator*(const V& arg) const {
        return Box(mins * arg, maxs * arg);
    }

    void operator*=(const V& arg) {
        mins *= arg;
        maxs *= arg;
    }

    // /
    Box operator/(const V& arg) const {
        return *this * (T(1) / arg);
    }

    Box& operator/=(const V& arg) {
        return *this *= (T(1) / arg);
    }

    // ==
    bool operator==(const Box& arg) const {
        return (mins == arg.mins) && (maxs == arg.maxs);
    }

    // Size
    V size() const {
        return maxs - mins;
    }

    bool is_empty() const {
        return any(maxs <= mins);
    }

    T width() const {
        return maxs.x - mins.x;
    }

    T height() const {
        return maxs.y - mins.y;
    }

    T depth() const {
        return maxs.z - mins.z;
    }

    // Adjustments
    friend Box expand(const Box& box, const V& arg) {
        return Box(box.mins - arg, box.maxs + arg);
    }

    friend Box shrink(const Box& box, const V& arg) {
        return Box(box.mins + arg, box.maxs - arg);
    }

    // Math
    V mix(const V& arg) const {
        return ply::mix(mins, maxs, arg);
    }

    V unmix(const V& arg) const {
        return ply::unmix(mins, maxs, arg);
    }

    V mid() const {
        return (mins + maxs) * 0.5f;
    }

    Box mix(const Box& arg) const {
        return {mix(arg.mins), mix(arg.maxs)};
    }

    Box unmix(const Box& arg) const {
        return {unmix(arg.mins), unmix(arg.maxs)};
    }

    V clamp(const V& arg) const {
        return clamp(arg, mins, maxs);
    }

    V top_left() const {
        return V{mins.x, maxs.y};
    }

    V bottom_right() const {
        return V{maxs.x, mins.y};
    }

    // Boolean operations
    bool contains(const V& arg) const {
        return all(mins <= arg) && all(arg < maxs);
    }

    bool contains(const Box& arg) const {
        return all(mins <= arg.mins) && all(arg.maxs <= maxs);
    }

    bool intersects(const Box& arg) const {
        return !intersect(*this, arg).is_empty();
    }

    static Box zero() {
        return {V{0}, V{0}};
    }

    static Box empty() {
        return {V{Limits<T>::Max}, V{Limits<T>::Min}};
    }

    static Box full() {
        return {V{Limits<T>::Min}, V{Limits<T>::Min}};
    }
};

template <typename V>
Box<V> make_union(const V& p0, const V& p1) {
    return Box<V>(min(p0, p1), max(p0, p1));
}

template <typename V>
Box<V> make_union(const Box<V>& box, const V& arg) {
    return Box<V>(min(box.mins, arg), max(box.maxs, arg));
}

template <typename V>
Box<V> make_union(const Box<V>& a, const Box<V>& b) {
    return Box<V>(min(a.mins, b.mins), max(a.maxs, b.maxs));
}

template <typename V>
Box<V> intersect(const Box<V>& a, const Box<V>& b) {
    return Box<V>(max(a.mins, b.mins), min(a.maxs, b.maxs));
}

template <typename V>
Box<V> make_solid(const Box<V>& a) {
    return Box<V>(min(a.mins, a.maxs), max(a.mins, a.maxs));
}

template <typename V>
Box<V> round_nearest(const Box<V>& a, float spacing = 1) {
    return {round_nearest(a.mins, spacing), round_nearest(a.maxs, spacing)};
}

template <typename V>
Box<V> round_expand(const Box<V>& a, float spacing = 1) {
    return {round_down(a.mins, spacing), round_up(a.maxs, spacing)};
}

} // namespace ply
