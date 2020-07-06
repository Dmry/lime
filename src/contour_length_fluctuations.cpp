#include "contour_length_fluctuations.hpp"
#include "parallel_policy.hpp"
#include "ics_log_utils.hpp"

#include <boost/math/special_functions/gamma.hpp>
#include <boost/math/quadrature/exp_sinh.hpp>
// #include <unsupported/Eigen/SpecialFunctions>

#include <algorithm>
#include <stdexcept>
#include <cmath>
#include <limits>

Contour_length_fluctuations::Contour_length_fluctuations(Time_series::time_type time_range_)
: Time_series(time_range_)
{}

// Has to be above call site to facilitate type deduction
inline auto
Contour_length_fluctuations::mu_t_functional(double Z, double tau_e, double G_f_normed, double tau_df)
{
    // p_star
    Summation<double> sum{1.0, std::sqrt(Z/10.0), 2.0};

    double e_star_ = e_star(Z, tau_e, G_f_normed);

    return [=](double t) mutable -> double {
        auto f = [=](const double& p){return 1.0/square(p) * exp( - t*square(p)/tau_df );};

        const double integ = integral_result(e_star_, t);

        const double res = (integ*0.306)/(Z*std::pow(tau_e, 0.25));

        return G_f_normed* sum(f) + res;
    };
}

void
Contour_length_fluctuations::update(const Context& ctx)
{
    std::transform(exec_policy, time_range_->begin(), time_range_->end(), values_.begin(), mu_t_functional(ctx.Z, ctx.tau_e, ctx.G_f_normed, ctx.tau_df));

    if (Async_except::get()->ep)
    {
        std::rethrow_exception(Async_except::get()->ep);
    }
}

double
Contour_length_fluctuations::e_star(double Z, double tau_e, double G_f_normed)
{
    Summation<double> sum{1.0, std::sqrt(Z/10.0), 2.0};

    constexpr auto f = [](const double& p) -> double {return 1.0/square(p);};

    return 1.0/(tau_e*std::pow(Z,4.0)) * std::pow((4.0*0.306 / (1-G_f_normed*sum(f)) ),4.0);
}

double
Contour_length_fluctuations::integral_result(double lower_bound, double t)
{
    auto f = [t] (double epsilon) -> double {
        return std::pow(epsilon, -5.0/4.0) * std::exp(-epsilon*t);
    };

    double res{0.0};

    try
    {
        using namespace boost::math::quadrature;
        exp_sinh<double> integrator;
        double termination = std::sqrt(std::numeric_limits<double>::epsilon());
        res = integrator.integrate(f, lower_bound , std::numeric_limits<double>::infinity(), termination, nullptr, nullptr, nullptr);
    }
    catch (const std::exception& ex)
    {
        Async_except::get()->ep = std::current_exception();
        BOOST_LOG_TRIVIAL(debug) << "In CR: " << ex.what();
    }

    return res;
}