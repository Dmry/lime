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

Register_class<IConstraint_release, RUB_constraint_release, constraint_release::impl, Time_series::time_type, double> rubinstein_constraint_release_factory(constraint_release::impl::RUBINSTEINCOLBY);

RUB_constraint_release::RUB_constraint_release(Time_series::time_type time_range, double c_v, size_t realizations) : IConstraint_release{time_range, c_v}, realizations_{realizations}, km(), prng{std::random_device{}()}, dist(0, 1)
{}

auto
RUB_constraint_release::R_t_functional(double Z, double tau_e, double G_f_normed, double tau_df)
{
    km.resize(Z);

    double E_star = e_star(Z, tau_e, G_f_normed);

    generate(G_f_normed, tau_df, tau_e, E_star, Z);

    return [this](double t) -> double {
        return integral_result(t);
    };
}

Time_series_functional::functional_type
RUB_constraint_release::time_functional(const Context& ctx)
{
    return R_t_functional(ctx.Z, ctx.tau_e, ctx.G_f_normed, ctx.tau_df);
}

void
RUB_constraint_release::update(const Context& ctx)
{
    std::transform(exec_policy, time_range_->begin(), time_range_->end(), values_.begin(), R_t_functional(ctx.Z, ctx.tau_e, ctx.G_f_normed, ctx.tau_df));

    if (Async_except::get()->ep)
    {
        std::rethrow_exception(Async_except::get()->ep);
    }
}

double
RUB_constraint_release::e_star(double Z, double tau_e, double G_f_normed)
{
    Summation sum{1.0, std::sqrt(Z/10.0), 2.0};

    constexpr auto f = [](const double& p) -> double {return 1.0/square(p);};

    return 1.0/(tau_e*std::pow(Z,4.0)) * std::pow((4.0*0.306 / (1.0-G_f_normed*sum(f)) ),4.0);
}

void
RUB_constraint_release::generate(double Gf_norm, double tau_df, double tau_e, double e_star, double Z)
{
    double p_star = std::sqrt(Z/10.0);

    km.resize(static_cast<size_t>(Z*realizations_));

    std::vector<double> rand(km.size());

    std::generate(rand.begin(), rand.end(), [this] (){ return dist(prng); });

    // minimize function depending on the random input
    auto minimize_functor = [&] (const double& rand_) {
        using namespace boost::math::tools; // For bracket_and_solve_root.

        auto f = [&](double epsilon_){return cp(Gf_norm, tau_df, tau_e, p_star, e_star, Z, epsilon_) -  rand_;};

        eps_tolerance<double> tol(std::numeric_limits<double>::digits - 2.0);

        std::pair<double, double> r;

        try
        {
            r = bisect(f, 0.0, 1e1, tol);
        }
        catch(const std::exception& ex)
        {
            BOOST_LOG_TRIVIAL(debug) << ex.what();
        }
        
        return (r.first + (r.second - r.first) / 2.0);
    };

    std::transform(exec_policy, rand.begin(), rand.end(), km.begin(), minimize_functor);
}

double
RUB_constraint_release::Me(double epsilon)
{
    double sum{0.0};
    size_t realization_size{km.size()/realizations_};

    for (size_t j = 0 ; j < realizations_ ; ++j)
    {
        size_t strides = j * realization_size;

        // Si needs to be local to avoid race conditions.
        std::vector<double> Si(realization_size-1);

        Si[0] = km[0+strides] + km[1+strides] - epsilon;

        // Race condition alert! No parallel execution here
        for (size_t i = 1+strides, s = 1 ; i < strides+Si.size() ; ++i, ++s)
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
RUB_constraint_release::integral_result(double t)
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
        BOOST_LOG_TRIVIAL(debug) << "In CR: " << ex.what();
    }

    return res;
}

double
RUB_constraint_release::cp(double Gf_norm, double tau_df, double tau_e, double p_star, double e_star, double Z,  double epsilon)
{
    return cp_one(Gf_norm, tau_df, p_star, epsilon) + cp_two(Z, tau_e, e_star, epsilon);
}

double
RUB_constraint_release::cp_one(double Gf_norm, double tau_df, double p_star, double epsilon)
{
    Summation<double> sum(1.0, 0.0, 2.0);

    auto f = [](const double& p) {return 1.0 / square(p);};
   
    double res{0.0};

    if (epsilon >= 1.0 / tau_df and epsilon < square(p_star)/tau_df)
    {
        sum.end = std::sqrt(epsilon*tau_df);
        res = Gf_norm * sum(f);
    }
    else if (epsilon >= square(p_star)/tau_df)
    {
        sum.end = p_star;
        res = Gf_norm * sum(f);
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