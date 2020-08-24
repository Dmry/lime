#pragma once

/*
 *  This file is part of Lime, a tool for the application of Likhtman & McLiesh' model for polymer dyanmics.
 *
 *  Copyright © 2020 Daniel Emmery (CNRS)
 *
 *  File contents:
 * 
 *  Post processing of model computation results.
 *  E.g. Mean square errors for fits and derivatives for validation
 *
 *  GPL 3.0 License
 * 
 */

#include <boost/version.hpp>

#if BOOST_VERSION >= 107000
#include <boost/math/differentiation/finite_difference.hpp>
#else
    #include <boost/math/tools/numerical_differentiation.hpp>
#endif

#include <gsl/gsl_errno.h>
#include <gsl/gsl_spline.h>

#include "time_series.hpp"
#include "parallel_policy.hpp"

#include <vector>
#include <array>
#include <tuple>
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

struct Schwarzl
{
    std::array<double, 10> interp;

    gsl_interp_accel *acc;
    gsl_spline *spline;

    std::vector<double> g_t_;
    std::vector<double> t_;

    Schwarzl(const std::vector<double>& g_t, const std::vector<double>& t)
    : g_t_{g_t}, t_{t}
    {
        acc = gsl_interp_accel_alloc();
        spline = gsl_spline_alloc (gsl_interp_cspline, g_t_.size());
        gsl_spline_init (spline, t_.data(), g_t_.data(), g_t_.size());
    }

    std::tuple<double, double, double>
    operator()(double t)
    {
        double current_t = t / 64.0;

        for (auto& val : interp)
        {
            if (current_t <= t_.front())
                val = g_t_.front();
            else if (current_t >= t_.back())
                val = g_t_.back();
            else
                val = gsl_spline_eval (spline, current_t, acc);

            current_t *= 2.0;
        }

        return {get_omega(t), Gp(), Gpp()};
    }

    double
    get_omega(double t)
    {
        return 1.0/t;
    }

    double
    Gp()
    {
        return
            interp[6]
            + 0.000451 * (interp[0] - interp[1])
            + 0.00716 * (interp[2] - interp[3])
            + 0.0010 * (interp[3] - interp[4])
            + 0.103 * (interp[4] - interp[5])
            + 0.099 * (interp[5] - interp[6])
            + 0.046 * (interp[6] - interp[7])
            + 0.717 * (interp[7] - interp[8])
            - 0.142 * (interp[8] - interp[9]);
    }

    double
    Gpp()
    {
        return
            0.03125 * 2.12 * (interp[0] - interp[1])
            + 0.0181 * (interp[1] - interp[2])
            + 0.0668 * (interp[2] - interp[3])
            + 0.191 * (interp[3] - interp[4])
            + 0.397 * (interp[4] - interp[5])
            + 0.412 * (interp[5] - interp[6])
            + 1.547 * (interp[6] - interp[7])
            - 0.441 * (interp[7] - interp[8]);
    }
};