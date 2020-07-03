#include <catch2/catch.hpp>

#include <limits>
#include <complex>

#include "../inc/inverse_laplace.hpp"

 TEST_CASE( "Inverse Laplace procedure yields good results for several known results." )
{
    auto f1 = [](const complex<double>& s) -> complex<double> { return (1.0/s); };
    auto f2 = [](const complex<double>& s) -> complex<double> { return (1.0/(s+2.0)); };

    REQUIRE( LaplaceInversion(f1,1.0,1e-8)-1.0 < 1e8 );

    REQUIRE( LaplaceInversion(f2,2.0,1e-8)-exp(-2.0*2.0) < 1e8  );
}