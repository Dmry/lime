#pragma once

#include "../context.hpp"
#include "../time_series.hpp"

#include <vector>

struct IConstraint_release : public Time_series, public Compute
{
    IConstraint_release(Time_series::time_type time_range, double c_v)
    :   Time_series(time_range),
        c_v_{c_v}
    {};

    virtual void update(const Context& ctx) = 0;
    double c_v_;
};