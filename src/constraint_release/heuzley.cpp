#include <boost/math/quadrature/exp_sinh.hpp>

#include "heuzley.hpp"
#include "../parallel_policy.hpp"
#include "../ics_log_utils.hpp"

#include <algorithm>
#include <cmath>

using namespace constraint_release;

IModel::IModel(const double Z_, const double tau_e_, const double tau_df_)
: Z{Z_}, tau_e{tau_e_}, tau_df{tau_df_}
{
    // Equation 16
    B1 = 3/2 * std::log(Z) - 1.63556;
    B2 = 3/4 * std::log(tau_e/tau_df) + B1;
}

Short::Short(const double Z_, const double tau_e_, const double tau_df_)
: IModel(Z_, tau_e_, tau_df_)
{}

double
Short::operator () (double epsilon) const
{
    double result{0.0};

    if (epsilon <= 1.0/tau_df)
    {
        result = low(epsilon);
    }
    else
    {
        result = high(epsilon);
    }
    
    return result;
}

double
Short::low(double epsilon) const
{
    return std::exp(B1)/std::sqrt(tau_e*epsilon);
}

double
Short::high(double epsilon) const
{
    return std::exp(B2)*std::pow(tau_e*epsilon, -5.0/4.0);
}

Medium::Medium(const double Z_, const double tau_e_, const double tau_df_)
: Short(Z_, tau_e_, tau_df_)
{
    n = 0.161*std::pow((Z-10.0), 0.2924) - 0.5;
    epsilon_B = std::pow(2.0, -(8*n+12.0)/11.0)*std::exp(20.0/11.0)*(1.0/tau_df);
}

double
Medium::operator () (double epsilon) const
{
    double result{0.0};

    if (epsilon <= 1.0/tau_df)
    {
        result = low(epsilon);
    }
    else if (epsilon > 1.0/tau_df and epsilon <= 4.0/tau_df)
    {
        result = medium_low(epsilon);
    }
    else if (epsilon > 4.0/tau_df and epsilon <= epsilon_B)
    {
        result = medium(epsilon);
    }
    else
    {
        result = high(epsilon);
    }
    
    return result;
}

double
Medium::medium_low(double epsilon) const
{
    return std::exp(B1)*std::pow(tau_e/tau_df,-1.0/2.0-n)*std::pow(tau_e*epsilon, n);
}

double
Medium::medium(double epsilon) const
{
    return std::pow(2.0, 2.0*n-3.0)*std::exp(B1-5.0)*std::pow(tau_e/tau_df,-2.0)*std::pow(tau_e*epsilon, 3.0/2.0);
}

Long::Long(const double Z_, const double tau_e_, const double tau_df_)
: Medium(Z_, tau_e_, tau_df_)
{
    epsilon_C = std::pow(2.0, -(8*n+12.0)/11.0)*std::exp(32.0/11.0)*(1.0/tau_df);
}

double
Long::operator () (double epsilon) const
{
    double result{0.0};

    if (epsilon <= 1.0/tau_df)
    {
        result = low(epsilon);
    }
    else if (epsilon > 1.0/tau_df and epsilon <= 4.0/tau_df)
    {
        result = medium_low(epsilon);
    }
    else if (epsilon > 4.0/tau_df and epsilon <= 16.0/tau_df)
    {
        result = medium(epsilon);
    }
    else if (epsilon > 16.0/tau_df and epsilon <= epsilon_C)
    {
        result = medium_high(epsilon);
    }
    else
    {
        result = high(epsilon);
    }
    
    return result;
}

double
Long::medium_high(double epsilon) const
{
    return std::pow(2, 2.0*n-3.0)*std::exp(B1-8.0)*std::pow(tau_e/tau_df,-2.0)*std::pow(tau_e*epsilon, 3.0/2.0);
}

Extra_long::Extra_long(const double Z_, const double tau_e_, const double tau_df_)
: Long(Z_, tau_e_, tau_df_)
{
    Long::n = 0.393;
}

HEU_constraint_release::HEU_constraint_release(Time_range::type time_range, double c_v)
: IConstraint_release{time_range, c_v}
{}


inline auto
HEU_constraint_release::R_t_functional(double Z, double tau_e, double tau_df)
{
    using namespace constraint_release;

    // Select model appropriate for chain length
    if (Z <= 10)
    {
        model.emplace<Short>(Z, tau_e, tau_df);
    }
    else if (Z > 10 and Z <= 160)
    {
        model.emplace<Medium>(Z, tau_e, tau_df);
    }
    else if (Z > 160 and Z <= 360)
    {
        model.emplace<Long>(Z, tau_e, tau_df);
    }
    else
    {
        model.emplace<Extra_long>(Z, tau_e, tau_df);
    }

    double epsilon_0 = epsilon_zero(Z, tau_e);

    return [=](double t) -> double {
        return tau_e*integral_result(epsilon_0, t);
    };
}

void
HEU_constraint_release::update(const Context& ctx)
{
    mtx.lock();

    std::transform(exec_policy, time_range_->begin(), time_range_->end(), R_t_.begin(), R_t_functional(ctx.Z, ctx.tau_e, ctx.tau_df));

    if (ep)
    {
        std::rethrow_exception(ep);
    }

    mtx.unlock();
}

double
HEU_constraint_release::integral_result(double lower_bound, double t)
{         
    auto f = [this, t] (double epsilon) -> double {
        return (*resolve(model))(epsilon)*std::exp(-epsilon*c_v_*t);
    };

    double res{0.0};

    try
    {
        using namespace boost::math::quadrature;
        exp_sinh<double> integrator;
        double termination = std::sqrt(std::numeric_limits<double>::epsilon());
        double L1;
        size_t levels;
        res = integrator.integrate(f, lower_bound, std::numeric_limits<double>::infinity(), termination, nullptr, &L1, &levels);
    }
    catch (const std::exception& ex)
    {
        ep = std::current_exception();
        BOOST_LOG_TRIVIAL(debug) << "In CR: " << ex.what();
    }

    return res;
}

// Equation 20, select lower bound for integration
inline double
HEU_constraint_release::epsilon_zero(double Z, double tau_e)
{
    if (Z < 25)
    {
        return 18.56*std::pow(Z, -4.664)/tau_e;
    }
    else // if (Z >= 25)
    {
        return 327.61*std::pow(Z, -5.602)/tau_e;
    }
}
