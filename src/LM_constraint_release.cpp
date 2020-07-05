#include "LM_constraint_release.hpp"
#include "parallel_policy.hpp"
#include "ics_log_utils.hpp"

#include <boost/math/quadrature/exp_sinh.hpp>
#include <boost/math/tools/roots.hpp>
#include <boost/math/differentiation/finite_difference.hpp>

#include <functional>
#include <memory>
#include <algorithm>
#include <utility>

LM_constraint_release::LM_constraint_release(double c_v_, Time_range::type time_range_, size_t realizations_) : R_t(time_range_->size()), time_range{time_range_}, c_v{c_v_}, realizations{realizations_}, km(), prng{std::random_device{}()}, dist(0, 1)
{}

auto
LM_constraint_release::R_t_functional()
{
    return [this](double t) -> double {
        return integral_result(t);
    };
}

void
LM_constraint_release::calculate(double Gf_norm, double tau_df, double tau_e, double e_star, double Z)
{
    km.resize(Z*realizations);

    generate(Gf_norm, tau_df, tau_e, e_star, Z);

    std::transform(exec_policy, time_range->begin(), time_range->end(), R_t.begin(), R_t_functional());

    if (ep)
    {
        std::rethrow_exception(ep);
    }

}

void
LM_constraint_release::generate(double Gf_norm, double tau_df, double tau_e, double e_star, double Z)
{
    double p_star = std::sqrt(Z/10.0);

    size_t size = static_cast<size_t>(Z*realizations);

    km.resize(size);

    std::vector<double> rand(size);

    std::generate(rand.begin(), rand.end(), [this] (){ return dist(prng); });

    // minimize function depending on the random input
    auto minimize_functor = [&] (const double& rand_) {

        using namespace std;  // Help ADL of std functions.
        using namespace boost::math::tools; // For bracket_and_solve_root.

        auto f = [&](double epsilon_){return cp(Gf_norm, tau_df, tau_e, p_star, e_star, Z, epsilon_) -  rand_;};


        double guess = 1.0/tau_e;
        double factor = 1.2;

        const boost::uintmax_t maxit = 1000;
        boost::uintmax_t it = maxit;
        bool is_rising = true;
        eps_tolerance<double> tol(std::numeric_limits<double>::digits - 2);

        std::pair<double, double> r = bracket_and_solve_root(f, guess, factor, is_rising, tol, it);

        return (r.first + (r.second - r.first) / 2.0);
    };

    std::transform(exec_policy, rand.begin(), rand.end(), km.begin(), minimize_functor);
}

double
LM_constraint_release::Me(double epsilon)
{
    double sum{0.0};
    size_t realization_size{km.size()/realizations};

    for (size_t j = 0 ; j < realizations ; ++j)
    {
        size_t strides = j * realization_size;

        std::vector<double> Si(realization_size-1);

        Si[0] = km[0+strides] + km[1+strides] - epsilon;

        // Race condition alert! No parallel execution here
        for (size_t i = 1+strides, s = 1 ; i < strides+Si.size() ; ++i, ++s)
            Si[s] = km[i] + km[i+1] - epsilon - square(km[i]) / Si[s-1];

        auto count = std::count_if(std::execution::seq, Si.begin(), Si.end(), [](const double& si) {return si < 0;});

        if (count > 0)
        {
            sum += static_cast<double>(count) /* / static_cast<double>(Si.size()) */;
        }
    }

    return sum / static_cast<double>(realizations);
}

double
LM_constraint_release::integral_result(double t)
{   
    auto me_wrapper = [this](const double& epsilon) {return Me(epsilon);};

    using namespace boost::math::differentiation;
    auto f = [this, t, me_wrapper] (double epsilon) -> double {
        return  finite_difference_derivative<decltype(me_wrapper), double, 8>(me_wrapper, epsilon) * std::exp(-epsilon*c_v*t);
     //   return Me(epsilon) * std::exp(-epsilon*c_v*t);
    };

    double res{0.0};

    try
    {
        using namespace boost::math::quadrature;
        exp_sinh<double> integrator;
        double termination = std::sqrt(std::numeric_limits<double>::epsilon());
        double L1;
        size_t levels;
        res = integrator.integrate(f, 0, std::numeric_limits<double>::infinity(), termination, nullptr, &L1, &levels);
    }
    catch (const std::exception& ex)
    {
        ep = std::current_exception();
        BOOST_LOG_TRIVIAL(debug) << "In CR: " << ex.what();
    }

    return res;
}

double
LM_constraint_release::cp(double Gf_norm, double tau_df, double tau_e, double p_star, double e_star, double Z,  double epsilon)
{
    return cp_one(Gf_norm, tau_df, p_star, epsilon) + cp_two(Z, tau_e, e_star, epsilon);
}

double
LM_constraint_release::cp_one(double Gf_norm, double tau_df, double p_star, double epsilon)
{
    Summation<double> sum(1.0, 0.0, 2.0);

    auto f = [](const double& p) {return 1.0 / square(p);};

    if (epsilon >= 1.0 / tau_df and epsilon < square(p_star)/tau_df)
    {
        sum.end = std::sqrt(epsilon*tau_df);
    }
    else if (epsilon >= square(p_star)/tau_df)
    {
        sum.end = p_star;
    }

    return Gf_norm * sum(f);
}

double
LM_constraint_release::cp_two(double Z, double tau_e, double e_star, double epsilon)
{
    if (epsilon < e_star)
    {
        return 0.0;
    }
    else
    {
        return 4*0.306 / (Z * std::pow(tau_e, 1.0/4.0)) * (std::pow(e_star, -1.0/4.0) - std::pow(epsilon, -1.0/4.0));
    }
}