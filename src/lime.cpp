#include <boost/log/utility/setup/console.hpp>
#include <boost/tuple/tuple.hpp>
#define BOOST_LOG_DYN_LINK 1

#include "../inc/args.hpp"

#include "parse.hpp"
#include "contour_length_fluctuations.hpp"
#include "constraint_release/heuzey.hpp"
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

struct cmd_writes_output_file
{
    Vector_writer<dat> writer;

    cmd_writes_output_file()
    {
        writer.path = "out.dat";
    }

    template<class F>
    void parse(F f)
    {
        f(writer.path,       "-o", "--output",                   args::help("Set the file path to write output to"),             args::required());
    }
};

struct result_cmd
{
    std::shared_ptr<Context> ctx;
    std::shared_ptr<System> system;
    
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
    }

    ICS_result build_driver(Time_series::time_type time)
    {
        std::unique_ptr<IContext_builder> builder = std::make_unique<ICS_context_builder>(system, ctx);

        ICS_result driver(time, builder.get());

        driver.CR->c_v_ = c_v;
        driver.context_->apply_physics();

        return driver;
    }
};

struct cmd_takes_file_input
{
    std::filesystem::path inpath;
    size_t file_col;

    cmd_takes_file_input() : inpath{}, file_col{1}
    {}

    template <class F>
    void parse(F f)
    {
        f(inpath,                                 args::help("Path to data file")                                );
        f(file_col, "-C", "--column",             args::help("Column to select from input file. Defaults to 1.") );
    }

    Time_series get_file_contents()
    {
        Time_series::value_type g_t;
        Time_series::time_type time;

        auto file_contents = parse_file<ics_file_format, clear_comments, store_headers>(inpath);

        g_t = Get<Time_series::value_primitive>::col(file_col, file_contents.out);
        time = Time_range::convert(Get<double>::col(0, file_contents.out));

        file_contents.buffer.clear();

        return Time_series(time, g_t);
    }
};

struct generate : lime::command<generate>, cmd_writes_output_file, result_cmd
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
        cmd_writes_output_file::parse(f);
        f(max_t,    "-t", "--maxtime",                      args::help("Maximum timestep for calculation"),              args::required());
        f(base,     "-b", "--base",                         args::help("Exponential growthfactor between steps")                         );
    }

    void run()
    {
        BOOST_LOG_TRIVIAL(info) << "Generating...";

        Time_range::type time = Time_range::generate_exponential(base, max_t);

        ICS_result driver = build_driver(time);

        driver.context_->print();

        writer.write(*time, driver.result());
    }
};

struct compare : lime::command<compare>, cmd_takes_file_input, result_cmd
{
    bool rmse;

    compare() : rmse{false}
    {}

    static constexpr const char* help()
    {
        return "Compare an existing G(t) curve to generated data.";
    }

    template<class F>
    void parse(F f)
    {
        cmd_takes_file_input::parse(f);
        result_cmd::parse(f);
        f(rmse,     "--rmse",                               args::help("Calculate RMSE between two G(t) curves"),         args::set(true));
    }

    void run()
    {
        BOOST_LOG_TRIVIAL(info) << "Generating...";

        auto input = get_file_contents();

        ICS_result driver = build_driver(input.get_time_range());

        if (rmse)
        {
            BOOST_LOG_TRIVIAL(info) << "RMSE: " << RMSE<Time_series::value_primitive>(input.get_values(), driver.result());
        }
    }
};


struct fit : lime::command<fit>, cmd_takes_file_input, cmd_writes_output_file, result_cmd
{
    double wt_pow;

    fit() : wt_pow{1.2}
    {}
    
    static const char* help()
    {
        return "Fit existing G(t) data with the Likhtman-Mcleish model";
    }

    template<class F>
    void parse(F f)
    {
        cmd_takes_file_input::parse(f);
        result_cmd::parse(f);
        cmd_writes_output_file::parse(f);
        f(wt_pow,   "-w", "--weightpower",        args::help("Set power for the weighting factor 1/(x^wt)")                             );
    }

    void run()
    {
        auto input = get_file_contents();

        auto driver = build_driver(input.get_time_range());

        //sigmoid_wrapper<double> cv_wrapper = driver.CR->c_v_;
        Fit<double, double> fit(driver.context_->N_e, driver.context_->tau_monomer);

        fit.fit(input.get_values(), driver, wt_pow);

        auto res = driver.result();

        driver.context_->print();

        BOOST_LOG_TRIVIAL(info) << "cv: " << driver.CR->c_v_;

        writer.write(*input.get_time_range(), res);
    }
};

struct reproduce : lime::command<reproduce>, cmd_writes_output_file
{
    std::shared_ptr<Context> ctx;

    double base;
    double max_t;

    bool du;
    bool dr;

    reproduce() : ctx{std::make_shared<Context>()}, base{1.2}, du{false}, dr{false}
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
        f(max_t,        "-t", "--time",                     args::help("Final timestep for calculation"),                       args::required());
        f(base,         "-b", "--base",                     args::help("Exponential growthfactor between steps")                                );
        f(du,           "-u", "--dmu",                      args::help("Reproduce dimensionless derivative of mu (figure 2)."),  args::set(true));
        f(dr,           "-R", "--dR",                       args::help("Reproduce dimensionless derivative of R  (figure 6)."),  args::set(true));
        cmd_writes_output_file::parse(f);
    } 

    Time_series dimensionless_series(Time_series_functional& functional, std::shared_ptr<Context> ctx)
    {
        auto func = functional.time_functional(*ctx);
        Time_series series = derivative(func, functional.get_time_range());
        std::for_each(exec_policy, series.time_zipped_begin(), series.time_zipped_end(), [&ctx](auto val) mutable -> double {
                return boost::get<1>(val) *= -4.0*ctx->Z*std::pow(ctx->tau_e, 0.25)*std::pow(boost::get<0>(val), 0.75);
        });
        return series;
    }

    void run()
    {
        CLF_context_builder builder(ctx);
        builder.gather_physics();

        ctx = builder.get_context();
        ctx->apply_physics();

        Time_series_functional::time_type normalized_time = Time_range::generate_exponential(base, max_t);

        std::for_each(exec_policy, normalized_time->begin(), normalized_time->end(), [this](Time_range::primitive& t){t /= ctx->tau_e;});

        std::list<std::pair<std::string, Time_series_functional*>> list;

        if (du) list.emplace_back("du", new Contour_length_fluctuations(normalized_time));
        if (dr) list.emplace_back("dr", new HEU_constraint_release(normalized_time, 1.0));

        for (auto& pair : list)
        {
            BOOST_LOG_TRIVIAL(info) << "Writing " << pair.first << "...";
            writer.path.replace_filename(pair.first + "_" + writer.path.filename().string());

            auto& series = *pair.second;

            auto result = dimensionless_series(series, ctx);
            writer.write(*result.get_time_range(), result.get_values());
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