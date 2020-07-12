#define BOOST_TEST_MAIN
#if !defined( WIN32 )
    #define BOOST_TEST_DYN_LINK
#endif

#define BOOST_TEST_MODULE test_integration
#include <boost/test/unit_test.hpp>

#include "../src/constraint_release/constraint_release.hpp"
#include "../src/time_series.hpp"
#include "../src/parse.hpp"
#include "../src/utilities.hpp"

#include <array>
#include <filesystem>

struct ics_file_format : file_format<ics_file_format>
{
    ics_file_format()
    {
        eol = "\r\n";
        delims = " ,";
    }
};

BOOST_AUTO_TEST_CASE( lm_figure_six )
{
    std::array<constraint_release::impl, 2> impls{constraint_release::impl::HEUZEY, constraint_release::impl::RUBINSTEINCOLBY};

    std::shared_ptr<Context> ctx = std::make_shared<Context>();
    ctx->Z = 300;
    ctx->tau_e = 1;

    CLF_context_builder builder(ctx);
    builder.gather_physics();

    ctx = builder.get_context();
    ctx->apply_physics();

    for (auto& pair : list)
    {
        BOOST_LOG_TRIVIAL(info) << "Computing " << pair.first << "...";
        writer.path.replace_filename(pair.first + "_" + original_filename);

        Time_series series = derivative(*pair.second, normalized_time);

        std::for_each(exec_policy, series.time_zipped_begin(), series.time_zipped_end(), [this](auto val) mutable -> double {
            double& time = boost::get<0>(val);
            double& value = boost::get<1>(val);
            
            return value *= -4.0*ctx->Z*std::pow(ctx->tau_e, 0.25)*std::pow(time, 0.75);
        });

        writer.write(*series.get_time_range(), series.get_values());
    }
}