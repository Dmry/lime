#include <boost/log/utility/setup/console.hpp>
#include <boost/tuple/tuple.hpp>
#define BOOST_LOG_DYN_LINK 1

#include "../inc/args.hpp"

#include "parse.hpp"
#include "contour_length_fluctuations.hpp"
#include "lime_log_utils.hpp"
#include "result.hpp"
#include "parallel_policy.hpp"
#include "writer.hpp"
#include "utilities.hpp"
#include "tube.hpp"
#include "fit.hpp"
#include "postprocess.hpp"

#include <list>
#include <filesystem>
#include <algorithm>
#include <tuple>

struct ics_file_format : file_format<ics_file_format>
{
    ics_file_format()
    {
        eol = "\r\n";
        delims = " ,";
    }
};

struct lime : args::group<lime>
{
    static constexpr const char* help()
    {
        return "Tool for the analysis of polymer thin films.\nAuthor: Daniel Emmery";
    }

    template<class F>
    void parse(F f)
    {
        auto debug = [](auto&&, const auto&, const args::argument&) {
            boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::debug);
        };

        f(nullptr,  "-v", "--version", args::help("Print version"),    args::show("0.0.1")         );
        f(nullptr,  "-D", "--debug",   args::help("Enable debugging"), args::lazy_callback(debug)  );
    }

    lime() {}

    void run() {}
};

struct result_cmd
{
    std::shared_ptr<Context> ctx;
    std::shared_ptr<System> system;
    std::filesystem::path outpath;
    double c_v;

    result_cmd() : ctx{std::make_shared<Context>()}, system{std::make_shared<System>()}, c_v{0.1}
    {
        system->T = 1.0;
    }

    template<class F>
    void parse(F f)
    {
        f(ctx->N,            "-N", "--length",                   args::help("Chain length"),                                     args::required());
        f(system->rho,       "-r", "--density",                  args::help("Density"),                                          args::required());
        f(system->T,         "-T", "--temperature",              args::help("Temperature")                                                       );
        f(ctx->N_e,          "-n", "--monomersperentanglement",  args::help("Number of monomers per entanglement"),              args::required());
        f(ctx->tau_monomer,  "-m", "--monomerrelaxationtime",    args::help("Initial guess of the monomer relaxation time"),     args::required());
        f(c_v,               "-c", "--crparameter",              args::help("Constraint release parameter"),                     args::required());
        f(outpath,           "-o", "--output",                   args::help("Set the file path to write output to"),             args::required());
    }
};

struct generate : lime::command<generate>, result_cmd
{
    double base;
    double max_t;
    
    generate() : base{1.2}
    {}

    static constexpr const char* help()
    {
        return "Generate G(t) curve for given parameters.";
    }

    template<class F>
    void parse(F f)
    {
        result_cmd::parse(f);
        f(max_t,    "-t", "--time",                         args::help("Final timestep for calculation"),                args::required());
        f(base,     "-b", "--base",                         args::help("Exponential growthfactor between steps")                         );
    }

    void run()
    {
        BOOST_LOG_TRIVIAL(info) << "Generating...";

        Time_range::type time = Time_range::generate_exponential(base, max_t);

        std::unique_ptr<IContext_builder> builder = std::make_unique<ICS_context_builder>(system, ctx);

        ICS_result driver(time, builder.get());
        driver.context_->apply_physics();
        driver.context_->print();

        Vector_writer<dat> writer;
        writer.path = outpath;
        writer.write(*time, driver.result());
    }
};

struct fit : lime::command<fit>, result_cmd
{
    std::filesystem::path inpath;
    size_t file_col;
    double wt_pow;

    fit() : inpath{}, file_col{1}, wt_pow{1.2}
    {}
    
    static const char* help()
    {
        return "Fit existing G(t) data with the Likhtman-Mcleish model";
    }

    template<class F>
    void parse(F f)
    {
        result_cmd::parse(f);
        f(inpath,                                 args::help("Path to data file")                                                       );
        f(file_col, "-C", "--column",             args::help("Column to select from input file"),                       args::required());
        f(wt_pow,   "-w", "--weightpower",        args::help("Set power for the weighting factor 1/(x^wt)")                             );
    }

    void run()
    {
        Time_series::value_type g_t;
        Time_series::time_type time;

        auto file_contents = parse_file<ics_file_format, clear_comments, store_headers>(inpath);

        g_t = Get<Time_series::value_primitive>::col(file_col, file_contents.out);
        time = Time_range::convert(Get<double>::col(0, file_contents.out));

        file_contents.buffer.clear();

        std::unique_ptr<IContext_builder> builder = std::make_unique<ICS_context_builder>(system, ctx);
        ICS_result driver(time, builder.get());

        driver.CR->c_v_ = c_v;

        sigmoid_wrapper<double> cv_wrapper = driver.CR->c_v_;
        Fit<double, double, sigmoid_wrapper<double>> fit(driver.context_->N_e, driver.context_->tau_monomer, cv_wrapper);

        fit.fit(g_t, driver, wt_pow);

        auto res = driver.result();

        driver.context_->print();

        BOOST_LOG_TRIVIAL(info) << "cv: " << driver.CR->c_v_;
        
        Vector_writer<dat> writer;

        if (not outpath.empty())
            writer.path = outpath;

        writer.write(*time, res);
    }
};

struct reproduce : lime::command<reproduce>
{
    std::shared_ptr<Context> ctx;
    std::filesystem::path outpath;

    double base;
    double max_t;

    bool du;

    reproduce() : ctx{std::make_shared<Context>()}, base{1.2}, du{false}
    {}

    static const char* help()
    {
        return "Reproduce results from the original Milner & McLeish paper.";
    }

    template<class F>
    void parse(F f)
    {
        f(ctx->Z,       "-Z", "--entanglementno",           args::help("Number of entanglements"),                              args::required());
        f(ctx->tau_e,   "-e", "--entanglementtime",         args::help("Entanglement time"),                                    args::required());
        f(outpath,      "-o", "--output",                   args::help("Set the file path to write output to"),                 args::required());
        f(max_t,        "-t", "--time",                     args::help("Final timestep for calculation"),                       args::required());
        f(base,         "-b", "--base",                     args::help("Exponential growthfactor between steps")                                );
        f(du,           "-u", "--dmu",                      args::help("Reproduce dimensionless derivative of mu (figure 2)."), args::set(true) );
    }

    void run()
    {
        CLF_context_builder builder(ctx);
        builder.gather_physics();

        ctx = builder.get_context();
        ctx->apply_physics();

        Time_range::type normalized_time = Time_range::generate_exponential(base, max_t);

        std::for_each(exec_policy, normalized_time->begin(), normalized_time->end(), [this](Time_range::primitive& t){t /= ctx->tau_e;});
        
        if (du)
        {
            BOOST_LOG_TRIVIAL(info) << "Writing d mu(t) to " << outpath;

            Vector_writer<dat> writer;
            writer.path = outpath;

            auto func = Contour_length_fluctuations::mu_t_functional(ctx->Z, ctx->tau_e, ctx->G_f_normed, ctx->tau_df);
            auto series = derivative(func, normalized_time);
            std::for_each(exec_policy, series.time_zipped_begin(), series.time_zipped_end(), [this](auto val) mutable -> double {
                return boost::get<1>(val) *= -4.0*ctx->Z*std::pow(ctx->tau_e, 0.25)*std::pow(boost::get<0>(val), 0.75);
            });

            writer.write(series.copy_time_range(), series.copy_values());
        }
    }
};

void ics_terminate() {
    BOOST_LOG_TRIVIAL(error) << "Unhandled exception";
    std::rethrow_exception(std::current_exception());
//  abort();  // forces abnormal termination
}

int main(int argc, char const *argv[])
{
    namespace log = boost::log;
    log::add_console_log(std::cout, log::keywords::format = lime_log::coloring_formatter);
    log::core::get()->set_filter(log::trivial::severity >= log::trivial::info);

    std::set_terminate (ics_terminate);

    try
    {
        args::parse<lime>(argc, argv);
    }
    catch (const std::runtime_error& err)
    {
        BOOST_LOG_TRIVIAL(error) << err.what();
        exit(1);
    }

    return 0;
}