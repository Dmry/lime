#include <catch2/catch.hpp>

#include "../src/constraint_release.hpp"

#include <gsl/gsl_errno.h>

#include "utilities.hpp"

TEST_CASE("CR fills R_t after update")
{
    double entanglement_time{10.0};
    double c_v{0.5};
    double Z = GENERATE(2.0, 50.0, 100.0, 200.0, 100.0);
    double tau_df{0.1};
    Time_range::type time_range = test_time_range();

    gsl_set_error_handler_off();

// TODO: CUSTOM ERROR HANDLER GSL

    Constraint_release cr(c_v, time_range);

    cr.calculate(Z, entanglement_time, tau_df);

    using Catch::Matchers::VectorContains;
    
 //   REQUIRE_THAT( cr.R_t, !VectorContains( NAN ) );
    REQUIRE_THAT( cr.R_t, !VectorContains( 0.0 ) );
}