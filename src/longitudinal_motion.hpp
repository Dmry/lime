#pragma once

#include "utilities.hpp"

struct Longitudinal_motion : private Summation<double, double>
{
    Longitudinal_motion(double Z, double tau_r)
        : Summation<double, double>(1.0, Z - 1.0, 1.0,
                                    [this](const double &p, const double &t) {
                                        return std::exp(-square(p) * t / tau_r_);
                                    }),
          Z_{Z}, tau_r_{tau_r}
    {}

    double operator() (double t)
    {
        end = Z_-1.0;
        auto sum = Summation<double, double>::operator()(t);
        return sum / (5.0 * Z_);
    }

    double Z_;
    double tau_r_;
};
