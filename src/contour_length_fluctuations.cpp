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

Contour_length_fluctuations::Contour_length_fluctuations()
: sum{1.0, 0.0, 2.0}
{}

Contour_length_fluctuations::Contour_length_fluctuations(Time_range::type time_range_)
: sum{1.0, 0.0, 2.0}, time_range{time_range_}, mu_t(time_range->size(), 0.0)
{}

// Has to be above call site to facilitate type deduction
inline auto
Contour_length_fluctuations::mu_t_functional(double Z, double tau_e, double G_f_normed, double tau_df)
{
    // p_star
    sum.end = std::sqrt(Z/10.0);

    double e_star_ = e_star(Z, tau_e, G_f_normed);

    return [=](double t) -> double {
        auto f = [=](const double& p){return 1.0/square(p) * exp( - t*square(p)/tau_df );};

        const double integ = integral_result(e_star_, t);

        const double res = (integ*0.306)/(Z*std::pow(tau_e, 0.25));

        return G_f_normed* sum(f) + res;
    };
}

void
Contour_length_fluctuations::update(const Context& ctx)
{
    std::transform(exec_policy, time_range->begin(), time_range->end(), mu_t.begin(), mu_t_functional(ctx.Z, ctx.tau_e, ctx.G_f_normed, ctx.tau_df));

    if (ep)
    {
        std::rethrow_exception(ep);
    }

}

double
Contour_length_fluctuations::e_star(double Z, double tau_e, double G_f_normed)
{
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
        double L1;
        size_t levels;
        res = integrator.integrate(f, lower_bound , std::numeric_limits<double>::infinity(), termination, nullptr, &L1, &levels);
    }
    catch (const std::exception& ex)
    {
        ep = std::current_exception();
        ICS_LOG(debug, "In CR: " + std::string(ex.what()));
    }

    return res;
}