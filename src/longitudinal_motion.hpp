#pragma once

#include "utilities.hpp"

struct Longitudinal_motion
{
    double operator() (double t)
    {
        Summation<double> sum{1.0, Z-1.0, 1.0, [this,&t](const double& p) {return std::exp(-square(p)*t/tau_r);}};
        return sum() / (5.0 * Z);
    }

    double Z;
    double tau_r;
};