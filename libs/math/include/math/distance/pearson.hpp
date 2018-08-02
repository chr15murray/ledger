#pragma once
#include "core/assert.hpp"
#include "math/correlation/pearson.hpp"
#include "math/shape_less_array.hpp"
#include "vectorise/memory/range.hpp"

#include <cmath>

namespace fetch {
namespace math {
namespace distance {

template <typename T, std::size_t S = memory::VectorSlice<T>::E_TYPE_SIZE>
inline typename memory::VectorSlice<T, S>::type Pearson(memory::VectorSlice<T, S> const &a,
                                                        memory::VectorSlice<T, S> const &b)
{
  using type = typename memory::VectorSlice<T, S>::type;
  return type(1) - correlation::Pearson(a, b);
}

template <typename T, typename C>
inline typename ShapeLessArray<T, C>::type Pearson(ShapeLessArray<T, C> const &a,
                                                   ShapeLessArray<T, C> const &b)
{
  return Pearson(a.data(), b.data());
}

}  // namespace distance
}  // namespace math
}  // namespace fetch