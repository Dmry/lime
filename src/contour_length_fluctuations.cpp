#include "contour_length_fluctuations.hpp"
#include "parallel_policy.hpp"
#include "lime_log_utils.hpp"

#include <boost/math/quadrature/exp_sinh.hpp>

#include <algorithm>
#include <stdexcept>
#include <cmath>
#include <limits>

Contour_length_fluctuations::Contour_length_fluctuations(double Z, double tau_e, double G_f_normed, double tau_df)
: Time_functor{}, G_f_normed_{G_f_normed}, e_star_{e_star(Z, tau_e, G_f_normed)}, norm_{norm(Z, tau_e)}, sum_{sum_term(Z, tau_df)}
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
    const double integ = integral_result(e_star_, t);

    const double res = (integ*0.306)/norm_;

    return G_f_normed_ * sum_(t) + res;
};

void
Contour_length_fluctuations::update(const Context& ctx)
{
    G_f_normed_ = ctx.G_f_normed;
    sum_ = sum_term(ctx.Z, ctx.tau_df);
    e_star_ = e_star(ctx.Z, ctx.tau_e, ctx.G_f_normed);
    norm_ = norm(ctx.Z, ctx.tau_e);
}

void
Contour_length_fluctuations::validate_update(const Context& ctx) const
{
    auto args = boost::hana::make_tuple(ctx.G_f_normed, ctx.tau_df, ctx.tau_e, ctx.Z);

    using namespace checks;
    using namespace checks::policies;

    try
    {
        static const std::string location = "in CLF update";
        check<decltype(args),
              is_nan<throws>, zero<throws>, negative<prints_warning_append<location>>>(args);
    }
    catch (const std::exception&)
    {
        std::throw_with_nested(std::runtime_error("in CLF update"));
    }
}

double
Contour_length_fluctuations::e_star(double Z, double tau_e, double G_f_normed)
{
    Summation<double> sum{1.0, std::sqrt(Z/10.0), 2.0, [](const double& p) -> double {return 1.0/square(p);}};

    return 1.0/(tau_e*std::pow(Z,4.0)) * std::pow((4.0*0.306 / (1.0-G_f_normed*sum()) ),4.0);
}

Contour_length_fluctuations::sum_t
Contour_length_fluctuations::sum_term(double Z, double tau_df)
{
    return sum_t{
        1.0, std::sqrt(Z/10.0), 2.0,
        [&, tau_df](const double& p, const double& t){return 1.0/square(p) * exp( -t*square(p)/tau_df );}
    };
}

double
Contour_length_fluctuations::norm(double Z, double tau_e)
{
    return Z*std::pow(tau_e, 0.25);
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
    catch (const std::exception&)
    {
        Async_except::get()->ep = std::current_exception();
    }

    return res;
}
