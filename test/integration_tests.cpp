#define BOOST_TEST_MAIN
#if !defined( WIN32 )
    #define BOOST_TEST_DYN_LINK
#endif

#define BOOST_TEST_MODULE test_integration
#include <boost/test/unit_test.hpp>

#include "../src/constraint_release/constraint_release.hpp"
#include "../src/constraint_release/heuzey.hpp"
#include "../src/time_series.hpp"
#include "../src/file_reader.hpp"
#include "../src/postprocess.hpp"

#include <array>
#include <filesystem>
#include <algorithm>

BOOST_AUTO_TEST_CASE( lm_figure_six )
{
    std::array<constraint_release::impl, 2> impls{/* constraint_release::impl::HEUZEY,  */constraint_release::impl::RUBINSTEINCOLBY};

    Time_series input = File_reader::get_file_contents("../validation/likhtmanmcleish/figure6_R.dat", 1);

    std::shared_ptr<Context> ctx = std::make_shared<Context>();
    ctx->Z = 300;
    ctx->tau_e = 1;

    CLF_context_builder builder(ctx);
    builder.gather_physics();

    ctx = builder.get_context();
    ctx->apply_physics();

    for (auto& impl : impls)
    {
        auto CR = constraint_release::Factory_with_context::create(impl, 0.1, *ctx);

        auto wrapper = [&CR](const Time_series::value_primitive& t) {return (*CR)(t);};

        Time_series series = derivative(wrapper, input.get_time_range());

        std::for_each(exec_policy, series.time_zipped_begin(), series.time_zipped_end(), [&ctx] (auto val) mutable -> double {
            double& time = boost::get<0>(val);
            double& value = boost::get<1>(val);

            return value *= -4.0*ctx->Z*std::pow(ctx->tau_e, 0.25)*std::pow(time, 0.75);
        });

        for (size_t i{0} ; i < series.size() ; ++i)
        {
            BOOST_CHECK_CLOSE(*(series.begin()+i), *(input.begin()+i), 1.0);
        }
    }
}