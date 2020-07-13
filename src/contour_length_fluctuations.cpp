#include "contour_length_fluctuations.hpp"
#include "parallel_policy.hpp"
#include "lime_log_utils.hpp"

#include <boost/math/special_functions/gamma.hpp>
#include <boost/math/quadrature/exp_sinh.hpp>
// #include <unsupported/Eigen/SpecialFunctions>

#include <algorithm>
#include <stdexcept>
#include <cmath>
#include <limits>

Contour_length_fluctuations::Contour_length_fluctuations(double Z, double tau_e, double G_f_normed, double tau_df)
: Time_functor{}, Z_{Z}, tau_e_{tau_e}, G_f_normed_{G_f_normed}, tau_df_{tau_df}, p_star_{std::sqrt(Z_/10.0)}, e_star_{e_star(Z, tau_e, G_f_normed)}
{}

Contour_length_fluctuations::Contour_length_fluctuations(Context& ctx)
: Contour_length_fluctuations(ctx.Z, ctx.tau_e, ctx.G_f_normed, ctx.tau_df)
{
    ctx.attach_compute(this);
}

Time_series Contour_length_fluctuations::operator()(const Time_series::time_type& time_range) const
{
    Time_series res{time_range};

    std::transform(exec_policy, time_range->begin(), time_range->end(), res.begin(), *this);

    if (Async_except::get()->ep)
    {
        std::rethrow_exception(Async_except::get()->ep);
    }

    return res;
}

Time_series::value_primitive Contour_length_fluctuations::operator()(const Time_series::time_primitive& t) const
{
    Summation<double> sum{1.0, p_star_, 2.0, [&](const double& p){return 1.0/square(p) * exp( -t*square(p)/tau_df_ );}};

    const double integ = integral_result(e_star_, t);

    const double res = (integ*0.306)/(Z_*std::pow(tau_e_, 0.25));

    return G_f_normed_* sum() + res;
};

void
Contour_length_fluctuations::update(const Context& ctx)
{
    Z_ = ctx.Z; tau_e_ = ctx.tau_e; G_f_normed_ = ctx.G_f_normed; tau_df_ = ctx.tau_df; p_star_ = std::sqrt(Z_/10.0); e_star_ = e_star(Z_, tau_e_, G_f_normed_);
}

double
Contour_length_fluctuations::e_star(double Z, double tau_e, double G_f_normed)
{
    Summation<double> sum{1.0, std::sqrt(Z/10.0), 2.0, [](const double& p) -> double {return 1.0/square(p);}};

    return 1.0/(tau_e*std::pow(Z,4.0)) * std::pow((4.0*0.306 / (1.0-G_f_normed*sum()) ),4.0);
}

double
Contour_length_fluctuations::integral_result(double lower_bound, double t)
{
    auto f = [t] (double epsilon) -> double {
        return std::exp(-epsilon*t) / std::pow(epsilon, 5.0/4.0);
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
    }

    return res;
}