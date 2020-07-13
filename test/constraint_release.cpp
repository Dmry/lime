#define BOOST_TEST_MAIN
#if !defined( WIN32 )
    #define BOOST_TEST_DYN_LINK
#endif

#define BOOST_TEST_MODULE test_constraint_release
#include <boost/test/unit_test.hpp>
#include <boost/test/included/unit_test.hpp>

#include "../src/constraint_release/constraint_release.hpp"
#include "../src/constraint_release/heuzey.hpp"
#include "../src/time_series.hpp"
#include "../src/file_reader.hpp"
#include "../src/postprocess.hpp"

#include <array>
#include <filesystem>
#include <algorithm>
#include <memory>

struct Reproduction_context {
	using path_impl_pair = std::pair<std::filesystem::path, constraint_release::impl>;

	Reproduction_context()
	: ctx{std::make_shared<Context>()}
	{
		ctx->Z = 300;
		ctx->tau_e = 1;

		CLF_context_builder builder(ctx);
    	builder.gather_physics();

    	ctx = builder.get_context();
    	ctx->apply_physics();
	}

	~Reproduction_context()
	{}

	void check_impl(std::filesystem::path path, constraint_release::impl impl)
	{
		Time_series input = File_reader::get_file_contents(path, 1);

        auto CR = constraint_release::Factory_with_context::create(impl, 1.0, *ctx);

        auto wrapper = [&CR](const Time_series::value_primitive& t) {return (*CR)(t);};

        Time_series series = dimensionless_derivative(wrapper, input.get_time_range(), ctx);

        auto vals = series.get_values();
        auto inputs = input.get_values();

        for (size_t i{0} ; i < series.size() ; ++i)
        {
            BOOST_TEST_INFO("index: " << i);
            BOOST_CHECK_CLOSE(vals[i], inputs[i], 8.0);
        }
	}

	std::shared_ptr<Context> ctx;
};


BOOST_FIXTURE_TEST_CASE(
    lm_figure_six,
    Reproduction_context,
    * boost::unit_test::label("validation")
    * boost::unit_test::label("rubinstein")
    * boost::unit_test::description("Test Rubinstein & Colby method for constraint release against literature data. (doi:10.1063/1.455620)"))
{
    check_impl("../validation/likhtmanmcleish/figure6_R.dat", constraint_release::impl::RUBINSTEINCOLBY);
}

BOOST_FIXTURE_TEST_CASE(
    heuzey_figure_five,
    Reproduction_context,
    * boost::unit_test::label("validation")
    * boost::unit_test::label("heuzey")
    * boost::unit_test::description("Test Heuzey method for constraint release against literature data. (doi:10.1002/app.20881)"))
{
	check_impl("../validation/heuzey/figure5_R.dat", constraint_release::impl::HEUZEY);
}

BOOST_AUTO_TEST_CASE(
    constraint_release_factory,
    * boost::unit_test::label("factory")
    * boost::unit_test::label("heuzey")
    * boost::unit_test::label("rubinstein")
    * boost::unit_test::description("Test if constraint release factory returns an instance for given entried in constraint_release::impl."))
{
    std::array<constraint_release::impl, 2> impls{constraint_release::impl::HEUZEY, constraint_release::impl::RUBINSTEINCOLBY};

    auto time_range = Time_range::generate_exponential(1.2, 1000);

    for (auto impl : impls)
    {
        BOOST_TEST_INFO(impl);
        auto CR = constraint_release::Factory::create(impl, 0.1);
        BOOST_CHECK( CR );
    }
}