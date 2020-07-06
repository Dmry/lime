#pragma once

#include <boost/math/differentiation/finite_difference.hpp>

#include <vector>
#include <functional>
#include <numeric>
#include <cmath>

template<typename T>
T RMSE(const std::vector<T>& first, const std::vector<T>& second)
{
    // Define error functor: (x-y)^2
    auto error_func = [](T a, T b)
    {
        auto error = a - b;
        return error*error;
    };

    // Sum over i (x_i - y_i)^2
    auto sum = std::transform_reduce(exec_policy, first.begin(), first.end(), second.begin(), 0.0, std::plus<>(), error_func);
    
    // Square root of average error
    return std::sqrt(sum / static_cast<T>(first.size()));
}