#include <catch2/catch.hpp>

#include "../src/ics_log_utils.hpp"
#include "../src/system.hpp"
#include "../src/chain.hpp"
#include "../src/driver.hpp"

#include "utilities.hpp"

TEST_CASE("Test drive the system.", "[driver]") {
    using system_t = System<tau_e_from_physics, G_e_from_physics>;
    using tube_t = Tube<from_chain, all_parameters>;
    using chain_t = Chain<storage_only>;

    Time_range::type time_range = test_exponential_time_range();

    chain_t chain {
        1.0,
        2.0,
        3.0,
        4.0
    };

    tube_t tube;

    Driver<system_t, tube_t, chain_t> driver(tube, chain, time_range);

    auto result = driver.result();

    using Catch::Matchers::VectorContains;
    using Catch::Matchers::Equals;

    // Check reult made it into the final vector
    REQUIRE_THAT( result, !VectorContains( 0.0 ) );

    // Check not NaN
    REQUIRE_THAT( result, Equals( result ) );
}
