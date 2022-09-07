/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>

// Define alias template ply::impl::ItemType<T> that models the item type used in a range-based
// for loop over an instance of T, according to the rules of
// https://en.cppreference.com/w/cpp/language/range-for

namespace ply {
namespace impl {

PLY_MAKE_WELL_FORMEDNESS_CHECK_1(HasBeginMember, std::declval<T0>().begin())
PLY_MAKE_WELL_FORMEDNESS_CHECK_1(HasBeginADL, begin(std::declval<T0>()))

template <typename, typename>
struct ItemTypeImpl;
template <typename T>
struct ItemTypeImpl<T, std::enable_if_t<std::is_array<T>::value>> {
    using Type = decltype(*std::declval<T>());
};
template <typename T>
struct ItemTypeImpl<T, std::enable_if_t<!std::is_array<T>::value && HasBeginMember<T>>> {
    using Type = decltype(*std::declval<T>().begin());
};
template <typename T>
struct ItemTypeImpl<
    T, std::enable_if_t<!std::is_array<T>::value && !HasBeginMember<T> && HasBeginADL<T>>> {
    using Type = decltype(*begin(std::declval<T>()));
};

template <typename T>
using ItemType = std::decay_t<typename ItemTypeImpl<std::remove_reference_t<T>, void>::Type>;

} // namespace impl
} // namespace ply
