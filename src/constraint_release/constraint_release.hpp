#pragma once

/*
 *  This file is part of Lime, a tool for the application of Likhtman & McLiesh' model for polymer dyanmics.
 *
 *  Copyright © 2020 Daniel Emmery (CNRS)
 *
 *  File contents:
 * 
 *  Prototype class and factory template for constraint release computation
 *
 *  GPL 3.0 License
 * 
 */

#include "../context.hpp"
#include "../time_series.hpp"
#include "../factory.hpp"
#include "../checks.hpp"

#include <vector>

struct IConstraint_release : public Time_functor, public Compute
{
    IConstraint_release(double c_v)
    :   Time_functor{},
        c_v_{c_v}
    {}

    virtual ~IConstraint_release() = default;

    virtual Time_series operator()(const Time_series::time_type&) const = 0;
    virtual Time_series::value_primitive operator()(const Time_series::time_primitive&) const = 0;
    
    virtual void update(const Context& ctx) = 0;
    virtual void validate_update(const Context& ctx) const = 0;
    double c_v_;
};

namespace constraint_release
{
    enum impl
    {
        HEUZEY,
        RUBINSTEINCOLBY,
        CLF
    };

    typedef Factory_template<IConstraint_release, impl, double, Context&> Factory_with_context;
}
