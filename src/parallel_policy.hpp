#pragma once

#include <execution>

#ifdef RUN_PARALLEL
    inline constexpr auto exec_policy = std::execution::par_unseq;
#else
    inline constexpr auto exec_policy = std::execution::seq;
#endif