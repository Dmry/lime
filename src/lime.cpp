#include <boost/log/utility/setup/console.hpp>
#include <boost/tuple/tuple.hpp>
#define BOOST_LOG_DYN_LINK 1

#include "config.h"

#include "../inc/args.hpp"

#include "file_reader.hpp"
#include "contour_length_fluctuations.hpp"
#include "constraint_release/heuzey.hpp"
#include "constraint_release/rubinsteincolby.hpp"
#include "constraint_release/clf.hpp"
#include "lime_log_utils.hpp"
#include "result.hpp"
#include "parallel_policy.hpp"
#include "writer.hpp"
#include "utilities.hpp"
#include "writer.hpp"
#include "tube.hpp"
#include "fit.hpp"
#include "postprocess.hpp"

#include <unordered_map>
#include <filesystem>
#include <algorithm>
#include <tuple>
#include <numeric>
#include <cmath>

static Register_class<IConstraint_release, RUB_constraint_release, constraint_release::impl, double, Context&> rubinstein_constraint_release_factory(constraint_release::impl::RUBINSTEINCOLBY);
static Register_class<IConstraint_release, HEU_constraint_release, constraint_release::impl, double, Context&> heuzey_constraint_release_factory(constraint_release::impl::HEUZEY);
static Register_class<IConstraint_release, CLF_constraint_release, constraint_release::impl, double, Context&> clf_constraint_release_factory(constraint_release::impl::CLF);

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

        f(nullptr, "-v", "--version", args::help("Print version"), args::show(PROJECT_VER));
        f(nullptr,  "-D", "--debug",   args::help("Enable debugging"), args::lazy_callback(debug)  );
    }

    lime() {}

    void run() {}
};

struct cmd_writes_output_file
{
    Writer writer;
    std::filesystem::path outpath;

    cmd_writes_output_file() : writer{"out.dat"}
    {}

    template<class F>
    void parse(F f)
    {
        auto set_file = [this] (auto&& val, const auto&, const args::argument&) {
            writer.set_path(val);
        };

        f(outpath,  "-o", "--output",   args::help("Set the file path to write output to"),     args::lazy_callback(set_file));
    }
};

struct result_cmd
{
    std::shared_ptr<Context> ctx;
    std::shared_ptr<System> system;
    std::unique_ptr<IContext_view> view;

    constraint_release::impl CR_impl;
    
    double c_v;

    result_cmd() : ctx{std::make_shared<Context>()}, system{std::make_shared<System>()}, CR_impl{constraint_release::impl::HEUZEY}, c_v{0.1}
    {
        system->T = 1.0;
    }

    template<class F>
    void parse(F f)
    {
        f(ctx->N,            "-N", "--length",                   args::help("Chain length"),                                 args::required());
        f(system->rho,       "-r", "--density",                  args::help("Density"),                                      args::required());
        f(system->T,         "-T", "--temperature",              args::help("Temperature")                                                   );
        f(ctx->N_e,          "-n", "--monomersperentanglement",  args::help("Number of monomers per entanglement"),          args::required());
        f(ctx->tau_monomer,  "-m", "--monomerrelaxationtime",    args::help("Initial guess of the monomer relaxation time"), args::required());
        f(c_v,               "-c", "--crparameter",              args::help("Constraint release parameter"),                 args::required());
        f(CR_impl,           "--rub",                            args::help("Enable Rubinstein&Colby constraint release"),   args::set(constraint_release::impl::RUBINSTEINCOLBY));
    }

    template <typename builder_t = ICS_context_builder>
    ICS_result build_result(Time_series::time_type time)
    {
        if (c_v <= 0.0)
        {
            BOOST_LOG_TRIVIAL(warning) << "cv value set to zero.";
        }

        std::unique_ptr<IContext_builder> builder = std::make_unique<builder_t>(system, ctx);

        ICS_result driver(time, builder.get(), CR_impl);

        view = builder->context_view();

        driver.CR->c_v_ = c_v;

        return driver;
    }
};

struct cmd_generates_exponential_timescale
{
    cmd_generates_exponential_timescale() : base{1.2}
    {}

    double max_t;
    double base;

    template<class F>
    void parse(F f)
    {
        auto check_base = [](auto&& data, const auto&, const args::argument&) {
            if ( std::abs(data - 1.0) < std::numeric_limits<double>::epsilon() )
            {
                throw std::runtime_error("Please enter a value other than 1.0 for flag base");
            }
        };

        f(max_t,    "-t", "--maxtime",      args::help("Maximum timestep for calculation"),         args::required()               );
        f(base,     "-b", "--base",         args::help("Exponential growthfactor between steps"),   args::lazy_callback(check_base));
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
        return File_reader::get_file_contents(inpath, file_col);
    }
};

struct generate : lime::command<generate>, cmd_writes_output_file, result_cmd, cmd_generates_exponential_timescale
{
    generate()
    {}

    static constexpr const char* help()
    {
        return "Generate G(t) curve for given parameters.";
    }

    template<class F>
    void parse(F f)
    {
        cmd_generates_exponential_timescale::parse(f);
        result_cmd::parse(f);
        cmd_writes_output_file::parse(f);
    }

    void run()
    {
        BOOST_LOG_TRIVIAL(info) << "Generating...";

        Time_range::type time = Time_range::generate_exponential(base, max_t);

        ICS_result result = build_result(time);

        BOOST_LOG_TRIVIAL(info) << *view;
        result.calculate();

        writer << *view << result;
    }
};

struct compare : lime::command<compare>, cmd_takes_file_input, result_cmd
{
    bool rmse;
    bool nrrmse;
    bool narmse;

    compare() : rmse{false}, nrrmse{false}, narmse{false}
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
        f(rmse,     "--rmse",                               args::help("Calculate RMSE between two G(t) curves"),          args::set(true));
        f(narmse,    "--narmse",                            args::help("Calculate RMSE normalized by average between two G(t) curves"),         args::set(true));
        f(nrrmse,    "--nrrmse",                            args::help("Calculate RMSE normalized by range between two G(t) curves"),           args::set(true));
    }

    void run()
    {
        auto input = get_file_contents();

        ICS_result result = build_result(input.get_time_range());

        result.calculate();

        if (rmse)
        {
            BOOST_LOG_TRIVIAL(info) << "RMSE: " << RMSE<Time_series::value_primitive>(result.get_values(), input.get_values());
        }
        if (narmse)
        {
            BOOST_LOG_TRIVIAL(info) << "NRMSE (by average): " << NRMSE_average<Time_series::value_primitive>(result.get_values(), input.get_values());
        }
        if (nrrmse)
        {
            BOOST_LOG_TRIVIAL(info) << "NRMSE (by range): " << NRMSE_range<Time_series::value_primitive>(result.get_values(), input.get_values());
        }
    }
};

struct fit : lime::command<fit>, cmd_takes_file_input, cmd_writes_output_file, result_cmd
{
    double wt_pow;
    bool decouple;

    fit() : wt_pow{1.2}, decouple{false}
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
        f(wt_pow,   "-w", "--weightpower",         args::help("Set power for the weighting factor 1/(x^wt)") );
        f(decouple, "--decouple",                  args::help("Decouple G_e and M_e"), args::set(true));
        f(ctx->G_e, "-g", "--entanglementmodulus", args::help("Set initial guess for entanglement modulus, only used when decouple=true") );
    }

    void write_output(const ICS_result& result)
    {
        BOOST_LOG_TRIVIAL(info) << *view << "cv: " << result.CR->c_v_;
        writer << *view << result;
    }

    void run()
    {
        auto input = get_file_contents();

        if (decouple)
        {
            auto result = build_result<ICS_decoupled_context_builder>(input.get_time_range());
            Fit fit_driver(result.context_->N_e, result.context_->tau_monomer, result.context_->G_e);
            fit_driver.fit(input.get_values(), result, wt_pow);
            write_output(result);
        }
        else
        {
            auto result = build_result<ICS_context_builder>(input.get_time_range());
            Fit fit_driver(result.context_->N_e, result.context_->tau_monomer);
            fit_driver.fit(input.get_values(), result, wt_pow);
            write_output(result);
        }
    }
};

struct reproduce : lime::command<reproduce>, cmd_writes_output_file, cmd_takes_file_input
{
    std::shared_ptr<Context> ctx;
    double c_v;

    std::unordered_map<std::string, Time_functor*> list;

    reproduce() : ctx{std::make_shared<Context>()}
    {}

    static const char* help()
    {
        return "Reproduce results from the original Milner & McLeish paper.";
    }

    template<class F>
    void parse(F f)
    {
        f(c_v,          "-c", "--cv",                       args::help("Constraint release parameter"),                         args::required());
        f(ctx->Z,       "-Z", "--entanglementno",           args::help("Number of entanglements"),                              args::required());
        f(ctx->tau_e,   "-e", "--entanglementtime",         args::help("Entanglement time"),                                    args::required());
        f(nullptr,       "--dmu",                         args::help("Reproduce dimensionless derivative of mu (figure 2)."),  args::lazy_callback(  [this](auto&&, auto&, auto&){list["du"] = new Contour_length_fluctuations(*ctx);}) );
        f(nullptr,       "--drrub",                       args::help("Reproduce dimensionless derivative of R using Rubinstein and Colby's method (figure 6)."),  args::lazy_callback([this](auto&&, auto&, auto&){list["dr_rub"] = new RUB_constraint_release(c_v, *ctx);}) );
        f(nullptr,       "--drheu",                       args::help("Reproduce dimensionless derivative of R using Rubinstein and Heuzey's method (figure 6)."),  args::lazy_callback([this](auto&&, auto&, auto&){list["dr_heu"] = new HEU_constraint_release(c_v, *ctx);}) );
        cmd_takes_file_input::parse(f);
        cmd_writes_output_file::parse(f);
    } 

    void run()
    {
        auto input = get_file_contents();
        auto original_filename = writer.get_path().filename().string();

        Reproduction_context_builder builder(ctx);

        for (auto& pair : list)
        {
            BOOST_LOG_TRIVIAL(info) << "Computing " << pair.first << "...";

            Derivative_result derivative(input.get_time_range(), &builder, *pair.second);
            derivative.calculate();

            writer.set_filename(pair.first + "_" + original_filename);

            writer << derivative;
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
#if BOOST_VERSION >= 107000
    log::add_console_log(std::cout, log::keywords::format = lime_log::coloring_formatter);
#else
    log::add_console_log(std::cout);
#endif
    log::core::get()->set_filter(log::trivial::severity >= log::trivial::info);

    std::set_terminate (ics_terminate);

    try
    {
        args::parse<lime>(argc, argv);
    }
    catch (const std::exception& err)
    {
        lime_log::print_exception(err);
        BOOST_LOG_TRIVIAL(fatal) << "Please validate and / or adjust your inputs and try again.";
        exit(1);
    }

    return 0;
}