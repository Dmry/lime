#pragma once

#include "../context.hpp"
#include "../utilities.hpp"

#include <vector>

struct IConstraint_release : public Compute
{
    IConstraint_release(Time_range::type time_range, double c_v)
    :   time_range_{time_range},
        R_t_(time_range_->size()),
        c_v_{c_v}
    {};

    virtual void update(const Context& ctx) = 0;

    std::vector<double> operator()() {return R_t_;}
    std::vector<double>::iterator begin() {return R_t_.begin();}
    std::vector<double>::iterator end() {return R_t_.end();}

    Time_range::type time_range_;
    std::vector<double> R_t_;
    double c_v_;
};