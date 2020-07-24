#include <boost/math/quadrature/exp_sinh.hpp>
#include <boost/math/tools/roots.hpp>
#include <boost/hana.hpp>

#include "../utilities.hpp"
#include "../parallel_policy.hpp"
#include "../lime_log_utils.hpp"
#include "../checks.hpp"

#include "rubinsteincolby.hpp"

#include <functional>
#include <memory>
#include <algorithm>
#include <cmath>
#include <utility>

RUB_constraint_release::RUB_constraint_release(double c_v, Context& ctx)
: RUB_constraint_release{c_v, ctx.Z, ctx.tau_e, ctx.G_f_normed, ctx.tau_df}
{
    ctx.attach_compute(this);
}

RUB_constraint_release::RUB_constraint_release(double c_v, double Z, double tau_e, double G_f_normed, double tau_df)
: IConstraint_release{c_v}, prng{std::random_device{}()}, dist(0, 1)
{
    const double E_star = e_star(Z, tau_e, G_f_normed);

    // Sets km and realizations_
    generate(G_f_normed, tau_df, tau_e, E_star, Z);
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
RUB_constraint_release::validate_update(const Context& ctx) const
{
    auto args = boost::hana::make_tuple(ctx.G_f_normed, ctx.tau_df, ctx.tau_e, ctx.Z);

    using namespace checks;
    using namespace checks::policies;

    try
    {
        static const std::string location = "in Rubinstein & Colby CR.";
        check<decltype(args),
              is_nan<throws>, zero<throws>, negative<prints_warning_append<location>>>(args);
    }
    catch (const std::exception& ex)
    {
        std::throw_with_nested(std::runtime_error("in RUB_cr update"));
    }
}

void
RUB_constraint_release::update(const Context& ctx)
{
    validate_update(ctx);

    const double E_star = e_star(ctx.Z, ctx.tau_e, ctx.G_f_normed);

    if (realization_size_ != static_cast<size_t>(ctx.Z))
    generate(ctx.G_f_normed, ctx.tau_df, ctx.tau_e, E_star, ctx.Z);
}

void
RUB_constraint_release::set_sizing_requirements(size_t Z)
{
    realization_size_ = Z;
    realizations_ = (Z > 100 ? Z*2 : 200);

    if (size_t required_size = Z*realizations_; km.size() != required_size)
    {
        km.resize(required_size);
    }
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
    set_sizing_requirements(static_cast<size_t>(Z));

    double p_star = std::sqrt(Z/10.0);

    std::generate(km.begin(), km.end(), [this] (){ return dist(prng); });

    // minimize function depending on the random input
    auto minimize_functor = [&] (double& rand_) {
        using namespace boost::math::tools; // For bracket_and_solve_root.

        auto f = [&](double epsilon_){return cp(G_f_normed, tau_df, tau_e, p_star, e_star, Z, epsilon_) -  rand_;};

        eps_tolerance<double> tol(std::numeric_limits<double>::digits);

        std::pair<double, double> r;

        try
        {
            r = bisect(f, 0.0, 1e10, tol);
        }
        catch(const std::exception& ex)
        {
        }
        
        rand_ = (r.first + (r.second - r.first) / 2.0);
    };

    std::for_each(exec_policy, km.begin(), km.end(), minimize_functor);

    #if defined CUDA && defined CUDA_FOUND
    cudetails.set_km(km);
    #endif
}

#if defined CUDA && defined CUDA_FOUND
double
RUB_constraint_release::Me(double epsilon) const
{
    return cudetails.cu_me(epsilon, static_cast<size_t>(Z_), realizations_);
}
#else
double
RUB_constraint_release::Me(double&& epsilon) const
{
    size_t sum{0};
    double s{0};

    for (size_t j = 0 ; j < realizations_ ; ++j)
    {
        const size_t stride = j * realization_size_;

        s = km[0+stride] + km[1+stride] - epsilon;
        if (s < 0.0) ++sum;

        for (size_t i = 1+stride; i < stride+realization_size_-1 ; ++i)
        {
            s = km[i] + km[i+1] - epsilon - square(km[i])/s;
            if (s < 0.0) ++sum;
        }
    }

    return static_cast<double>(sum) / static_cast<double>(km.size());
}
#endif

double
RUB_constraint_release::integral_result(double t) const
{   
    // Integration by parts
    auto f = [this, t] (double epsilon) -> double {
        return Me(std::forward<double>(epsilon)) * exp(-epsilon*c_v_*t);
    };

    double res{0.0};

    try
    {
        using namespace boost::math::quadrature;
        exp_sinh<double> integrator;
        double termination = std::sqrt(std::numeric_limits<double>::epsilon());
        res = c_v_ * t * integrator.integrate(f, 0, std::numeric_limits<double>::infinity(), termination, nullptr, nullptr, nullptr);
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