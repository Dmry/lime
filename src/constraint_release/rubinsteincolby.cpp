#include <boost/math/quadrature/exp_sinh.hpp>
#include <boost/math/tools/roots.hpp>

#include "../utilities.hpp"
#include "../parallel_policy.hpp"
#include "../lime_log_utils.hpp"

#include "rubinsteincolby.hpp"

#include <functional>
#include <memory>
#include <algorithm>
#include <cmath>
#include <utility>

Register_class<IConstraint_release, RUB_constraint_release, constraint_release::impl, double, Context&> rubinstein_constraint_release_factory(constraint_release::impl::RUBINSTEINCOLBY);

RUB_constraint_release::RUB_constraint_release(double c_v, Context& ctx, size_t realizations)
: RUB_constraint_release{c_v, ctx.Z, ctx.tau_e, ctx.G_f_normed, ctx.tau_df, realizations}
{
    ctx.attach_compute(this);
}

RUB_constraint_release::RUB_constraint_release(double c_v, double Z, double tau_e, double G_f_normed, double tau_df, size_t realizations)
: IConstraint_release{c_v}, Z_{Z}, tau_e_{tau_e}, G_f_normed_{G_f_normed}, tau_df_{tau_df}, realizations_{realizations}, km(static_cast<size_t>(Z_*realizations_)), prng{std::random_device{}()}, dist(0, 1)
{
    const double E_star = e_star(Z_, tau_e_, G_f_normed_);
    generate(G_f_normed_, tau_df_, tau_e_, E_star, Z_);
}

Time_series RUB_constraint_release::operator()(const Time_series::time_type& time_range) const
{
    Time_series res{time_range};

    std::transform(exec_policy, time_range->begin(), time_range->end(), res.begin(), std::ref(*this));

    if (Async_except::get()->ep)
    {
        std::rethrow_exception(Async_except::get()->ep);
    }

    return res;
}

Time_series::value_primitive RUB_constraint_release::operator()(const Time_series::time_primitive& t) const
{
    return integral_result(t);
}

void
RUB_constraint_release::update(const Context& ctx)
{
    Z_ = ctx.Z; tau_e_ = ctx.tau_e; G_f_normed_ = ctx.G_f_normed; tau_df_ = ctx.tau_df;
    km.resize(static_cast<size_t>(Z_*realizations_));
    const double E_star = e_star(Z_, tau_e_, G_f_normed_);
    generate(G_f_normed_, tau_df_, tau_e_, E_star, Z_);
}

double
RUB_constraint_release::e_star(double Z, double tau_e, double G_f_normed)
{
    Summation<double> sum{1.0, std::sqrt(Z/10.0), 2.0, [](const double& p) -> double {return 1.0/square(p);}};

    return 1.0/(tau_e*std::pow(Z,4.0)) * std::pow((4.0*0.306 / (1.0-G_f_normed*sum()) ),4.0);
}

void
RUB_constraint_release::generate(double G_f_normed, double tau_df, double tau_e, double e_star, double Z)
{
    double p_star = std::sqrt(Z/10.0);

    std::generate(km.begin(), km.end(), [this] (){ return dist(prng); });

    // minimize function depending on the random input
    auto minimize_functor = [&] (const double& rand_) {
        using namespace boost::math::tools; // For bracket_and_solve_root.

        auto f = [&](double epsilon_){return cp(G_f_normed, tau_df, tau_e, p_star, e_star, Z, epsilon_) -  rand_;};

        eps_tolerance<double> tol(std::numeric_limits<double>::digits - 2.0);

        std::pair<double, double> r;

        try
        {
            r = bisect(f, 0.0, 1e1, tol);
        }
        catch(const std::exception& ex)
        {
            //BOOST_LOG_TRIVIAL(debug) << ex.what();
        }
        
        return (r.first + (r.second - r.first) / 2.0);
    };

    std::for_each(exec_policy, km.begin(), km.end(), minimize_functor);
}

double
RUB_constraint_release::Me(double epsilon) const
{
    double sum{0.0};
    size_t realization_size{static_cast<size_t>(Z_)};

    for (size_t j = 0 ; j < realizations_ ; ++j)
    {
        size_t stride = j * realization_size;

        // Si needs to be local to avoid race conditions.
        std::vector<double> Si(realization_size-1);

        Si[0] = km[0+stride] + km[1+stride] - epsilon;

        // Race condition alert! No parallel execution here
        for (size_t i = 1+stride, s = 1 ; i < stride+Si.size() ; ++i, ++s)
            Si[s] = km[i] + km[i+1] - epsilon - square(km[i]) / Si[s-1];

        // par_unseq count_if is horrendously slow on the test system.
        // Using sequential execution for robustness (wrt speed) with
        // virtually no performance penalty.
        auto count = std::count_if(std::execution::seq, Si.begin(), Si.end(), [](const double& si) {return si < 0;});

        if (count > 0)
        {
            sum += static_cast<double>(count);
        }
    }

    return sum / static_cast<double>(km.size());
}

double
RUB_constraint_release::integral_result(double t) const
{   
    // Integration by parts
    auto f = [this, t] (double epsilon) -> double {
        return Me(epsilon) * std::exp(-epsilon*c_v_*t);
    };

    double res{0.0};

    try
    {
        using namespace boost::math::quadrature;
        exp_sinh<double> integrator;
        double termination = std::sqrt(std::numeric_limits<double>::epsilon());
        res = c_v_*t * integrator.integrate(f, 0, std::numeric_limits<double>::infinity(), termination, nullptr, nullptr, nullptr);
    }
    catch (const std::exception& ex)
    {
        Async_except::get()->ep = std::current_exception();
    }

    return res;
}

double
RUB_constraint_release::cp(double G_f_normed, double tau_df, double tau_e, double p_star, double e_star, double Z,  double epsilon)
{
    return cp_one(G_f_normed, tau_df, p_star, epsilon) + cp_two(Z, tau_e, e_star, epsilon);
}

double
RUB_constraint_release::cp_one(double G_f_normed, double tau_df, double p_star, double epsilon)
{
    Summation<double> sum(1.0, 0.0, 2.0, [](const double& p) {return 1.0 / square(p);});
   
    double res{0.0};

    if (epsilon >= 1.0 / tau_df and epsilon < square(p_star)/tau_df)
    {
        sum.end = std::sqrt(epsilon*tau_df);
        res = G_f_normed * sum();
    }
    else if (epsilon >= square(p_star)/tau_df)
    {
        sum.end = p_star;
        res = G_f_normed * sum();
    }

    return res;
}

double
RUB_constraint_release::cp_two(double Z, double tau_e, double e_star, double epsilon)
{
    if (epsilon < e_star)
    {
        return 0.0;
    }
    else
    {
        return 4.0*0.306 / (Z * std::pow(tau_e, 1.0/4.0)) * (std::pow(e_star, -1.0/4.0) - std::pow(epsilon, -1.0/4.0));
    }
}