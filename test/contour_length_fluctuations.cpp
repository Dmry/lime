#include <catch2/catch.hpp>

#include "../src/contour_length_fluctuations.hpp"

#include "utilities.hpp"

#include <vector>

TEST_CASE("CLF fills mu_t after update") {
    double entanglement_time = test_entanglement_time();
    double df = test_df();
    double Z = test_entanglement_time();
    Time_range::type t = test_time_range();

    Contour_length_fluctuations CLF(t);

    CLF.calculate(Z, entanglement_time, df);

    using Catch::Matchers::VectorContains;

    REQUIRE_THAT( CLF.mu_t, !VectorContains( 0.0 ) );
}