#pragma once

/*
 *  This file is part of Lime, a tool for the application of Likhtman & McLiesh' model for polymer dyanmics.
 *
 *  Copyright © 2020 Daniel Emmery (CNRS)
 *
 *  File contents:
 * 
 *  Computation of the rouse modes term
 *
 *  GPL 3.0 License
 * 
 */

#include "context.hpp"
#include "utilities.hpp"
#include "time_series.hpp"
#include "parallel_policy.hpp"

struct Rouse_motion : private Summation<double, double>
{
    Rouse_motion(double Z, double tau_r, double N)
        : Summation<double, double>(Z, N, 1.0,
                                    [this](const double &p, const double &t) {
                                        return std::exp(-2.0 * square(p) * t / tau_r_);
                                    }),
          Z_{Z}, tau_r_{tau_r}, N_{N}
    {}

    double operator() (double t)
    {
        start = Z_;
        end = N_;
        auto sum = Summation<double, double>::operator()(t);
        return sum / Z_;
    }

    Time_series operator()(const Time_series::time_type &time_range) const
    {
        Time_series res{time_range};

        std::transform(exec_policy, time_range->begin(), time_range->end(), res.begin(), *this);

        return res;
    }

    double Z_;
    double tau_r_;
    double N_;
};