#include <boost/log/utility/setup/console.hpp>
#include <boost/tuple/tuple.hpp>
#define BOOST_LOG_DYN_LINK 1

#include "../inc/args.hpp"

#include "file_reader.hpp"
#include "contour_length_fluctuations.hpp"
#include "constraint_release/heuzey.hpp"
#include "constraint_release/rubinsteincolby.hpp"
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
#include <numeric>
#include <cmath>

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

    ICS_result build_driver(Time_series::time_type time)
    {
        std::unique_ptr<IContext_builder> builder = std::make_unique<ICS_context_builder>(system, ctx);

        ICS_result driver(time, builder.get(), CR_impl);

        driver.CR->c_v_ = c_v;
        driver.context_->apply_physics();

        return driver;
    }
};

struct cmd_generates_exponential_timescale
{
    cmd_generates_exponential_timescale() : base{1.2}
    {

    }

    double max_t;
    double base;

    template<class F>
    void parse(F f)
    {
        auto check_base = [](auto&& data, const auto&, const args::argument&) {
            if ( std::abs(data - 1.0) < std::numeric_limits<double>::epsilon )
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

        ICS_result driver = build_driver(input.get_time_range());

        auto res = driver.result();

        if (rmse)
        {
            BOOST_LOG_TRIVIAL(info) << "RMSE: " << RMSE<Time_series::value_primitive>(res, input.get_values());
        }
        if (narmse)
        {
            BOOST_LOG_TRIVIAL(info) << "NRMSE (by average): " << NRMSE_average<Time_series::value_primitive>(res, input.get_values());
        }
        if (nrrmse)
        {
            BOOST_LOG_TRIVIAL(info) << "NRMSE (by range): " << NRMSE_range<Time_series::value_primitive>(res, input.get_values());
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

        Fit<double, double> fit(driver.context_->N_e, driver.context_->tau_monomer);

        fit.fit(input.get_values(), driver, wt_pow);

        auto res = driver.result();

        driver.context_->print();

        BOOST_LOG_TRIVIAL(info) << "cv: " << driver.CR->c_v_;

        writer.write(*input.get_time_range(), res);
    }
};

struct reproduce : lime::command<reproduce>, cmd_writes_output_file, cmd_takes_file_input
{
    std::shared_ptr<Context> ctx;
    double c_v;

    using list_pair = std::pair<std::string, Time_functor*>;
    std::list<list_pair> list;

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
        f(nullptr,       "--dmu",                         args::help("Reproduce dimensionless derivative of mu (figure 2)."),  args::lazy_callback(  [this](auto&&, auto&, auto&){list.push_back(list_pair("du", new Contour_length_fluctuations(*ctx)));}) );
        f(nullptr,       "--drrub",                       args::help("Reproduce dimensionless derivative of R using Rubinstein and Colby's method (figure 6)."),  args::lazy_callback(  [this](auto&&, auto&, auto&){list.push_back(list_pair("dr_rub", new RUB_constraint_release(c_v, *ctx)));}) );
        f(nullptr,       "--drheu",                       args::help("Reproduce dimensionless derivative of R using Rubinstein and Heuzey's method (figure 6)."),  args::lazy_callback(  [this](auto&&, auto&, auto&){list.push_back(list_pair("dr_heu", new HEU_constraint_release(c_v, *ctx)));}) );
        cmd_takes_file_input::parse(f);
        cmd_writes_output_file::parse(f);
    } 

    void run()
    {
        CLF_context_builder builder(ctx);
        builder.gather_physics();

        ctx = builder.get_context();
        ctx->apply_physics();

        auto input = get_file_contents();

        auto original_filename = writer.path.filename().string();

        for (auto& pair : list)
        {
            BOOST_LOG_TRIVIAL(info) << "Computing " << pair.first << "...";
            writer.path.replace_filename(pair.first + "_" + original_filename);

            auto wrapper = [&pair](const double& t) {return (*pair.second)(t);};

            Time_series series = derivative(wrapper, input.get_time_range());

            std::for_each(exec_policy, series.time_zipped_begin(), series.time_zipped_end(), [this](auto val) mutable -> double {
                double& time = boost::get<0>(val);
                double& value = boost::get<1>(val);
                
                return value *= -4.0*ctx->Z*std::pow(ctx->tau_e, 0.25)*std::pow(time, 0.75);
            });

            writer.write(*series.get_time_range(), series.get_values());
        }
    }
};
