#pragma once

#include <boost/phoenix/function/adapt_function.hpp>

#include "context.hpp"
#include "utilities.hpp"

struct Rouse_motion
{
    auto modes (const double& t, const double& tau_r)
    {
        return [&tau_r,&t](const double& p) {return std::exp(-2.0*square(p)*t/tau_r);};
    }

    double operator() (double t)
    {
        Summation<double> sum{Z, N};
        return sum( modes(t,tau_r) ) / Z;
    }

    double Z;
    double tau_r;
    double N;
};