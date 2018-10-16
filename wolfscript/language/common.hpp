#pragma once

#include <type_traits>

namespace wolfscript
{

namespace detail
{

// Add const if the conditional is true
template <bool Tcond, typename T>
using add_const_cond_t = std::conditional_t<Tcond, std::add_const_t<T>, T>;


} // namespace detail

} // namespace wolfscript