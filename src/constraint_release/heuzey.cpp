#include <boost/math/quadrature/exp_sinh.hpp>

#include "heuzey.hpp"
#include "../parallel_policy.hpp"
#include "../lime_log_utils.hpp"

#include <algorithm>
#include <cmath>

Register_class<IConstraint_release, HEU_constraint_release, constraint_release::impl, double, Context&> heuzey_constraint_release_factory(constraint_release::impl::HEUZEY);

using namespace heuzey_detail;

HEU_constraint_release::HEU_constraint_release(double c_v, Context& ctx)
: HEU_constraint_release(c_v, ctx.Z, ctx.tau_e, ctx.tau_df)
{
    ctx.attach_compute(this);
}

HEU_constraint_release::HEU_constraint_release(double c_v, double Z, double tau_e, double tau_df)
: IConstraint_release{c_v}, Z_{Z}, tau_e_{tau_e}, tau_df_{tau_df}, model{get_model(Z_, tau_e_, tau_df_)}
{}

HEU_constraint_release::HEU_constraint_release(const HEU_constraint_release& other)
: IConstraint_release{other.c_v_}, Z_{other.Z_}, tau_e_{other.tau_e_}, tau_df_{other.tau_df_}, model{get_model(other.Z_, other.tau_e_, other.tau_df_)}
{}

Time_series HEU_constraint_release::operator()(const Time_series::time_type& time_range) const
{
    Time_series res{time_range};
    
    std::transform(exec_policy, time_range->begin(), time_range->end(), res.begin(), std::ref(*this));

    if (Async_except::get()->ep)
    {
        std::rethrow_exception(Async_except::get()->ep);
    }

    return res;
}

Time_series::value_primitive HEU_constraint_release::operator()(const Time_series::time_primitive& t) const
{
    double epsilon_0 = epsilon_zero(Z_, tau_e_);

    return tau_e_*integral_result(epsilon_0, t);
}

void
HEU_constraint_release::update(const Context& ctx)
{
    Z_ = ctx.Z; tau_e_ = ctx.tau_e; tau_df_ = ctx.tau_df; model = get_model(Z_, tau_e_, tau_df_);
}

// TODO: should really create a predicated factory for this
HEU_constraint_release::Model_ptr
HEU_constraint_release::get_model(double Z, double tau_e, double tau_df)
{
    Model_ptr ptr;

    // Select model appropriate for chain length
    if (Z <= 10.0)
    {
        ptr = std::make_unique<Short>(Z, tau_e, tau_df);
    }
    else if (Z > 10.0 and Z <= 160.0)
    {
        ptr = std::make_unique<Medium>(Z, tau_e, tau_df);
    }
    else if (Z > 160.0 and Z_ <= 360.0)
    {
        ptr = std::make_unique<Long>(Z, tau_e, tau_df);
    }
    else
    {
        ptr = std::make_unique<Extra_long>(Z, tau_e, tau_df);
    }

    return ptr;
}

double
HEU_constraint_release::integral_result(double lower_bound, double t) const
{         
    auto f = [this, t] (double epsilon) -> double {
        return (*model)(epsilon)*std::exp(-epsilon*c_v_*t);
    };

    double res{0.0};

    try
    {
        using namespace boost::math::quadrature;
        exp_sinh<double> integrator;
        double termination = std::sqrt(std::numeric_limits<double>::epsilon());
        res = integrator.integrate(f, lower_bound, std::numeric_limits<double>::infinity(), termination, nullptr, nullptr, nullptr);
    }
    catch (const std::exception& ex)
    {
        Async_except::get()->ep = std::current_exception();
    }

    return res;
}

// Equation 20, select lower bound for integration
inline double
HEU_constraint_release::epsilon_zero(double Z, double tau_e) const
{
    if (Z < 25.0)
    {
        return 18.56*std::pow(Z, -4.664)/tau_e;
    }
    else // if (Z >= 25)
    {
        return 327.61*std::pow(Z, -5.602)/tau_e;
    }
}

IModel::IModel(const double Z_, const double tau_e_, const double tau_df_)
: Z{Z_}, tau_e{tau_e_}, tau_df{tau_df_}
{
    // Equation 16
    B1 = 3.0/2.0 * std::log(Z) - 1.63556;
    B2 = 3.0/4.0 * std::log(tau_e/tau_df) + B1;
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
    return std::exp(B1)*std::pow(tau_e*epsilon, -1.0/2.0);
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
    epsilon_B = std::pow(2.0, (-8.0*n+12.0)/11.0)*std::exp(20.0/11.0)*(1.0/tau_df);
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
    epsilon_C = std::pow(2.0, (-8.0*n+12.0)/11.0)*std::exp(32.0/11.0)*(1.0/tau_df);
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
    return std::pow(2.0, 2.0*n-3.0)*std::exp(B1-8.0)*std::pow(tau_e/tau_df,-2.0)*std::pow(tau_e*epsilon, 3.0/2.0);
}

Extra_long::Extra_long(const double Z_, const double tau_e_, const double tau_df_)
: Long(Z_, tau_e_, tau_df_)
{
    n = 0.393;
}
