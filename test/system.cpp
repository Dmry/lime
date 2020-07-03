#include <catch2/catch.hpp>

#include <limits>

#include "../src/system.hpp"
#include "../src/chain.hpp"

TEST_CASE("System correctly sets free variable", "[system]")
{
/*     static Free_variable<double> entanglement_time{10.0};
    static Free_variable<double> G_e{5.0};

    Chain chain
    {
        10.0, // mass
        5.0,  // length
        1.0,  // persistence length
        2.0,  // kuhn segment length
    };

    std::vector<double> time{2.0, 4.0, 8.0, 16.0, 32.0, 64.0, 128.0, 256.0};

    System<entanglement_time, G_e> system(chain, time);

    // TODO: MANUAL CHECK
    REQUIRE( system.tau_e == Approx(0.008443432) ); */
}