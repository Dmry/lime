#pragma once

#include "time_series.hpp"
#include "parallel_policy.hpp"

#include <vector>
#include <functional>
#include <numeric>
#include <cmath>
#include <algorithm>

template<typename T>
T RMSE(const std::vector<T>& fit, const std::vector<T>& original)
{
    // Define error functor: (x-y)^2
    auto error_func = [](T a, T b)
    {
        auto error = a - b;
        return error*error;
    };

    // Sum over i (x_i - y_i)^2
    auto sum = std::transform_reduce(exec_policy, fit.begin(), fit.end(), original.begin(), 0.0, std::plus<>(), error_func);
    
    // Square root of average error
    return std::sqrt(sum / static_cast<T>(fit.size()));
}

template<typename T>
T NRMSE_range(const std::vector<T>& fit, const std::vector<T>& original)
{
    T rmse = RMSE<T>(fit, original);

    const auto [min, max] = std::minmax_element(original.begin(), original.end()); 

    return rmse / (*max - *min);
}

template<typename T>
T NRMSE_average(const std::vector<T>& fit, const std::vector<T>& original)
{
    T rmse = RMSE<T>(fit, original);

    const auto avg = std::accumulate(original.begin(), original.end(), 0.0) / static_cast<T>(original.size()); 

    return rmse / avg;
}



Time_series derivative(Time_series_functional::functional_type functional, Time_series_functional::time_type time_range);