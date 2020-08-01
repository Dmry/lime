#pragma once

#include <boost/version.hpp>

#if BOOST_VERSION >= 107000
#include <boost/math/differentiation/finite_difference.hpp>
#else
    #include <boost/math/tools/numerical_differentiation.hpp>
#endif

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

template<typename Functor_t>
Time_series derivative(const Functor_t& func, const Time_series::time_type& time_range)
{

#if BOOST_VERSION >= 107000
    using namespace boost::math::differentiation;
#else
    using namespace boost::math::tools;
#endif

    Time_series result(time_range);

    std::transform(exec_policy, time_range->begin(), time_range->end(), result.begin(), [&func](const Time_series::time_primitive &t) {
        return finite_difference_derivative<Functor_t, Time_series::time_primitive, 8>(func, t);
    });

    return result;
}

template<typename Functor_t>
Time_series dimensionless_derivative(const Functor_t& func, const Time_series::time_type& time_range, std::shared_ptr<Context> ctx)
{
    auto derivative_result = derivative(func, time_range);

    std::for_each(exec_policy, derivative_result.time_zipped_begin(), derivative_result.time_zipped_end(), [ctx](auto val) mutable -> double {
        double& time = boost::get<0>(val);
        double& value = boost::get<1>(val);
                
        return value *= -4.0*ctx->Z*std::pow(ctx->tau_e, 0.25)*std::pow(time, 0.75);
    });

    return derivative_result;
}