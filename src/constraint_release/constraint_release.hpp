#pragma once

#include "../context.hpp"
#include "../time_series.hpp"
#include "../factory.hpp"

#include <vector>

struct IConstraint_release : public Time_functor, public Compute
{
    IConstraint_release(double c_v)
    :   Time_functor{},
        c_v_{c_v}
    {};

    virtual Time_series operator()(const Time_series::time_type&) const = 0;
    virtual Time_series::value_primitive operator()(const Time_series::time_primitive&) const = 0;
    
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

    typedef Factory_template<IConstraint_release, impl, double, Context&> Factory_with_context;
    typedef Factory_template<IConstraint_release, impl, double> Factory;
}
