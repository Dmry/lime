#include <catch2/catch.hpp>
#include <vector>

#include "../src/system.hpp"
#include "../src/fit.hpp"
#include "../src/driver.hpp"

#include "utilities.hpp"

#include <any>

TEST_CASE( "Test fit" )
{
    using system_t = System<tau_e_free, G_e_free>;
    using tube_t = Tube<given_M_e, from_chain>;
    using chain_t = Chain<storage_only>;
    using driver_t = Driver<system_t, tube_t, chain_t>;

    Time_range::type time_range = test_exponential_time_range();

    tube_t tube
    {
        1.0,                    
        test_segment_count(),   // Z, range tested
        5.0                     // M_e is free
    };

    chain_t chain
    {
        0.001,                   // Mass, should be unused in this case
        1024
    };

    driver_t driver(tube, chain, time_range);

    driver.sys.G_e = GENERATE(0.5);
    driver.sys.tau_e = GENERATE(100.0, 1000.0);

    auto res_before = driver.result();

    driver.sys.G_e = GENERATE(0.5);
    driver.sys.tau_e = GENERATE(50.0, 100.0, 200.0, 1000.0);
    driver.tube.M_e = GENERATE(1.0, 5.0, 10.0);
    INFO("Z before: " << driver.tube.Z);
    INFO("Me before: " << driver.tube.M_e);
    INFO("Ge before: " << driver.sys.G_e);
    INFO("tau_e before: " << driver.sys.tau_e);

    auto free_variables = std::forward_as_tuple(driver.tube.M_e, driver.sys.G_e, driver.sys.tau_e);

    std::any data = driver_t::User_data
    {
        res_before.size(),
        res_before.data(),
        free_variables,
        &driver
    };

    Fit<driver_t::User_data, 3> fit;

    fit.fit(res_before.size(), data);

    driver.print();

    auto res_after = driver.result();

    using Catch::Matchers::Approx;
    using Catch::Matchers::Equals;

    INFO("Z after: " << driver.tube.Z);
    INFO("Me after: " << driver.tube.M_e);
    INFO("Ge after: " << driver.sys.G_e);
    INFO("tau_e after: " << driver.sys.tau_e);

    REQUIRE_THAT( res_after, Approx( res_before ) );

    // Check no NaN
    REQUIRE_THAT( res_after, Equals( res_after ) );
}
