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

struct Rouse_motion : private Summation<double, double>
{
    Rouse_motion(double Z, double tau_r, double N)
        : Summation<double, double>(1.0, Z - 1.0, 1.0,
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

    double Z_;
    double tau_r_;
    double N_;
};