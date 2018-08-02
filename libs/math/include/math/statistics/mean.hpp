#pragma once
#include "core/assert.hpp"
#include "math/shape_less_array.hpp"
#include "vectorise/memory/range.hpp"

#include <cmath>

namespace fetch {
namespace math {
namespace statistics {

template <typename A>
inline typename A::type Mean(A const &a)
{
  using vector_register_type = typename A::vector_register_type;
  using type                 = typename A::type;

  type ret = a.data().in_parallel().Reduce(
      memory::TrivialRange(0, a.size()),
      [](vector_register_type const &a, vector_register_type const &b) { return a + b; });
  ret /= a.size();

  return ret;
}

}  // namespace statistics
}  // namespace math
}  // namespace fetch