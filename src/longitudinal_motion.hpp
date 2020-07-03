#pragma once

#include "utilities.hpp"

struct Longitudinal_motion
{
    auto modes (const double& t, const double& tau_r)
    {
        return [&tau_r,&t](const double& p) {return std::exp(-square(p)*t/tau_r);};
    }

    double operator() (double t)
    {
        Summation<double> sum{1.0, Z-1.0};
        return sum( modes(t,tau_r) ) / (5.0 * Z);
    }

    double Z;
    double tau_r;
};