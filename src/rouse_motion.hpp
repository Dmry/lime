#pragma once

#include <boost/phoenix/function/adapt_function.hpp>

#include "context.hpp"
#include "utilities.hpp"

struct Rouse_motion
{
    double operator() (double t)
    {
        Summation<double> sum{Z, N, 1.0, [this,& t] (const double& p)->double {return std::exp(-2.0*square(p)*t/tau_r);}};
        return sum() / Z;
    }

    double Z;
    double tau_r;
    double N;
};