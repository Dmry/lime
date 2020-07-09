#pragma once

#include "../context.hpp"
#include "../time_series.hpp"
#include "../factory.hpp"

#include <vector>

struct IConstraint_release : public Time_series_functional, public Compute
{
    IConstraint_release(Time_series::time_type time_range, double c_v)
    :   Time_series_functional(time_range),
        c_v_{c_v}
    {};

    // Provides a generalized interface for all time functionals
    virtual Time_series_functional::functional_type time_functional(const Context& ctx) = 0;
    
    virtual void update(const Context& ctx) = 0;
    double c_v_;
};

namespace constraint_release
{
    enum impl
    {
        HEUZEY,
        RUBINSTEINCOLBY,
    };

    typedef Factory_template<IConstraint_release, impl, Time_series::time_type, double> Factory;
}
