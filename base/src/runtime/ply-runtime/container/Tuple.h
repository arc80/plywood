/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>

namespace ply {

template <typename...>
struct Tuple;

template <typename A, typename B>
static constexpr bool IsSameOrConst() {
    return std::is_same<A, B>::value || std::is_same<A, const B>::value;
}

//-----------------------------------------------------
// 2-tuple
//-----------------------------------------------------
template <typename T1, typename T2>
struct Tuple<T1, T2> {
    T1 first;
    T2 second;

    Tuple() = default;
    // Note: We don't bother using SFINAE to conditionally disable constructors depending on whether
    // types are actually copy-constructible or move-constructible.
    // We also don't bother to support constructing from types other than const T& or T&&.
    // May need to make some of these conditionally explicit, though.
    Tuple(const T1& first, const T2& second) : first{first}, second{second} {
    }
    Tuple(const T1& first, T2&& second) : first{first}, second{std::move(second)} {
    }
    Tuple(T1&& first, const T2& second) : first{std::move(first)}, second{second} {
    }
    Tuple(T1&& first, T2&& second) : first{std::move(first)}, second{std::move(second)} {
    }
    template <typename U1, typename U2>
    Tuple(const Tuple<U1, U2>& other) : first{other.first}, second{other.second} {
    }
    template <typename U1, typename U2>
    Tuple(Tuple<U1, U2>&& other) : first{std::move(other.first)}, second{std::move(other.second)} {
    }
    template <typename U1, typename U2>
    void operator=(const Tuple<U1, U2>& other) {
        first = other.first;
        second = other.second;
    }
    template <typename U1, typename U2>
    void operator=(Tuple<U1, U2>&& other) {
        first = std::move(other.first);
        second = std::move(other.second);
    }
    bool operator==(const Tuple& other) const {
        return first == other.first && second == other.second;
    }
    template <typename U1, typename U2,
              std::enable_if_t<IsSameOrConst<T1, U1>() && IsSameOrConst<T2, U2>(), int> = 0>
    operator Tuple<U1, U2>&() {
        return reinterpret_cast<Tuple<U1, U2>&>(*this);
    }
    template <typename U1, typename U2,
              std::enable_if_t<IsSameOrConst<T1, U1>() && IsSameOrConst<T2, U2>(), int> = 0>
    operator const Tuple<U1, U2>&() const {
        return reinterpret_cast<const Tuple<U1, U2>&>(*this);
    }
};

//-----------------------------------------------------
// 3-tuple
//-----------------------------------------------------
template <typename T1, typename T2, typename T3>
struct Tuple<T1, T2, T3> {
    T1 first;
    T2 second;
    T3 third;

    Tuple() = default;
    Tuple(const T1& first, const T2& second, const T3& third)
        : first{first}, second{second}, third{third} {
    }
    Tuple(const T1& first, const T2& second, T3&& third)
        : first{first}, second{second}, third{std::move(third)} {
    }
    Tuple(const T1& first, T2&& second, const T3& third)
        : first{first}, second{std::move(second)}, third{third} {
    }
    Tuple(const T1& first, T2&& second, T3&& third)
        : first{first}, second{std::move(second)}, third{std::move(third)} {
    }
    Tuple(T1&& first, const T2& second, const T3& third)
        : first{std::move(first)}, second{second}, third{third} {
    }
    Tuple(T1&& first, const T2& second, T3&& third)
        : first{std::move(first)}, second{second}, third{std::move(third)} {
    }
    Tuple(T1&& first, T2&& second, const T3& third)
        : first{std::move(first)}, second{std::move(second)}, third{third} {
    }
    Tuple(T1&& first, T2&& second, T3&& third)
        : first{std::move(first)}, second{std::move(second)}, third{std::move(third)} {
    }
    template <typename U1, typename U2, typename U3>
    Tuple(const Tuple<U1, U2, U3>& other)
        : first{other.first}, second{other.second}, third{other.third} {
    }
    template <typename U1, typename U2, typename U3>
    Tuple(Tuple<U1, U2, U3>&& other)
        : first{std::move(other.first)}, second{std::move(other.second)}, third{std::move(
                                                                              other.third)} {
    }
    template <typename U1, typename U2, typename U3>
    void operator=(const Tuple<U1, U2, U3>& other) {
        first = other.first;
        second = other.second;
        third = other.third;
    }
    template <typename U1, typename U2, typename U3>
    void operator=(Tuple<U1, U2, U3>&& other) {
        first = std::move(other.first);
        second = std::move(other.second);
        third = std::move(other.third);
    }
    bool operator==(const Tuple& other) const {
        return first == other.first && second == other.second && third == other.third;
    }
    template <
        typename U1, typename U2, typename U3,
        std::enable_if_t<
            IsSameOrConst<T1, U1>() && IsSameOrConst<T2, U2>() && IsSameOrConst<T3, U3>(), int> = 0>
    operator Tuple<U1, U2, U3>&() {
        return reinterpret_cast<Tuple<U1, U2, U3>&>(*this);
    }
    template <
        typename U1, typename U2, typename U3,
        std::enable_if_t<
            IsSameOrConst<T1, U1>() && IsSameOrConst<T2, U2>() && IsSameOrConst<T3, U3>(), int> = 0>
    operator const Tuple<U1, U2, U3>&() const {
        return reinterpret_cast<const Tuple<U1, U2, U3>&>(*this);
    }
};

} // namespace ply
