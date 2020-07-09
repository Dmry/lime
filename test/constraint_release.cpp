#define BOOST_TEST_MAIN
#if !defined( WIN32 )
    #define BOOST_TEST_DYN_LINK
#endif

#define BOOST_TEST_MODULE test_constraint_release
#include <boost/test/unit_test.hpp>

#include "../src/constraint_release/constraint_release.hpp"
#include "../src/time_series.hpp"

#include <array>

BOOST_AUTO_TEST_CASE( constraint_release_factory )
{
    std::array<constraint_release::impl, 2> impls{constraint_release::impl::HEUZEY, constraint_release::impl::RUBINSTEINCOLBY};

    auto time_range = Time_range::generate_exponential(1.2, 1000);

    for (auto impl : impls)
    {
        auto CR = constraint_release::Factory::create(impl, time_range, 0.1);
        BOOST_CHECK( CR );
    }
}