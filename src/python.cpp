/*
 *  This file is part of Lime, a tool for the application of Likhtman & McLiesh' model for polymer dyanmics.
 *
 *  Copyright © 2020 Daniel Emmery (CNRS)
 *
 *  File contents:
 * 
 *  Python API
 *
 *  GPL 3.0 License
 * 
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>

#include <functional>

#include "context.hpp"
#include "tube.hpp"
#include "fit.hpp"
#include "result.hpp"
#include "time_series.hpp"
#include "constraint_release/constraint_release.hpp"
#include "constraint_release/rubinsteincolby.hpp"
#include "constraint_release/heuzey.hpp"
#include "constraint_release/clf.hpp"

// A few wrappers to make things easier:

Time_range::base generate_exponential_wrapper(Time_range::primitive base, Time_range::primitive max)
{
    return *Time_range::generate_exponential(base, max);
}

// TODO: Support getting time series from result. Right now time is usually known by the user though (at least in the gui).
std::vector<double> extract_time_from_result(const ICS_result& result)
{
    return *result.get_time_range();
}

void fit(bool decouple, ICS_result &result, const std::vector<double> &input, double weighting, std::function<void()>& python_callback)
{
    auto cb =
        [](const size_t iter, void *userdata, const gsl_multifit_nlinear_workspace *w) {
            Fit<double, double, double>::default_callback(iter, nullptr, w);
            auto python_callback = static_cast<std::function<void()>*>(userdata);
            (*python_callback)();
        };

    if (decouple)
    {
        Fit fit_driver(result.context_->N_e, result.context_->tau_monomer, result.context_->G_e);
        fit_driver.callback_func = cb;
        fit_driver.callback_params = static_cast<void*>(&python_callback);
        fit_driver.fit(input, result, weighting);
    }
    else
    {
        Fit fit_driver(result.context_->N_e, result.context_->tau_monomer);
        fit_driver.callback_func = cb;
        fit_driver.callback_params = static_cast<void *>(&python_callback);
        fit_driver.fit(input, result, weighting);
    }
}       

std::string context_view_to_comment(IContext_view& view)
{
    std::ostringstream stream;
    view.serialize(stream, "# ");
    return stream.str();
}

PYBIND11_MODULE(lime_python, m)
{
    pybind11::class_<System, std::shared_ptr<System>>(m, "System")
        .def(pybind11::init([]() {
            return std::make_shared<System>();
        }))
        .def_readwrite("temperature", &System::T)
        .def_readwrite("friction", &System::friction)
        .def_readwrite("density", &System::rho);

    pybind11::class_<Context, std::shared_ptr<Context>>(m, "Context")
        .def(pybind11::init([]() {
            return std::make_shared<Context>();
        }))
        .def_readwrite("M", &Context::M)
        .def_readwrite("N", &Context::N)
        .def_readwrite("N_e", &Context::N_e)
        .def_readwrite("M_e", &Context::M_e)
        .def_readwrite("Z", &Context::Z)
        .def_readwrite("a", &Context::a)
        .def_readwrite("b", &Context::b)
        .def_readwrite("G_e", &Context::G_e)
        .def_readwrite("G_f_normed", &Context::G_f_normed)
        .def_readwrite("tau_e", &Context::tau_e)
        .def_readwrite("tau_d_0", &Context::tau_d_0)
        .def_readwrite("tau_r", &Context::tau_r)
        .def_readwrite("tau_df", &Context::tau_df)
        .def_readwrite("tau_monomer", &Context::tau_monomer);

    pybind11::class_<IContext_view>(m, "Context_view");

    pybind11::class_<ICS_context_builder>(m, "Context_builder")
        .def(pybind11::init<std::shared_ptr<System>, std::shared_ptr<Context>>())
        .def("get_context", &ICS_context_builder::get_context)
        .def("get_system", &ICS_context_builder::get_system)
        .def("get_context_view", &ICS_context_builder::context_view);

    pybind11::class_<ICS_decoupled_context_builder, ICS_context_builder>(m, "Decoupled_context_builder")
        .def(pybind11::init<std::shared_ptr<System>, std::shared_ptr<Context>>())
        .def("get_context", &ICS_decoupled_context_builder::get_context)
        .def("get_context_view", &ICS_decoupled_context_builder::context_view);

    pybind11::enum_<constraint_release::impl>(m, "cr_impl")
        .value("CLF", constraint_release::impl::CLF)
        .value("Heuzey", constraint_release::impl::HEUZEY)
        .value("Rubinsteincolby", constraint_release::impl::RUBINSTEINCOLBY);

    m.def(
        "init_factories",
        []() { Register_class<IConstraint_release, HEU_constraint_release, constraint_release::impl, double, Context &> heu_factory(constraint_release::impl::HEUZEY);
               Register_class<IConstraint_release, RUB_constraint_release, constraint_release::impl, double, Context &> rub_factory(constraint_release::impl::RUBINSTEINCOLBY);
               Register_class<IConstraint_release, CLF_constraint_release, constraint_release::impl, double, Context&> clf_factory(constraint_release::impl::CLF);
        },
        pybind11::return_value_policy::automatic);

    m.def("generate_exponential", &generate_exponential_wrapper, pybind11::return_value_policy::copy);

    pybind11::class_<ICS_result>(m, "ICS_result")
        .def(pybind11::init([](const Time_range::base& time, ICS_context_builder *builder, constraint_release::impl impl) {
            return ICS_result(std::make_shared<Time_range::base>(time), builder, impl);
        }))
        .def("calculate", &ICS_result::calculate)
        .def("get_values", &ICS_result::get_values)
        .def("set_cv", [](const ICS_result& res, double cv) {res.CR->c_v_ = cv;});

    m.def("fit", &fit);

    m.def("context_view_to_comment", &context_view_to_comment);

    /*
    Currently, this seems to cause lifetime issues. we'll use a wrapper for now.

    pybind11::class_<Fit<double, double, double>>(m, "Fit_decoupled")
        .def(pybind11::init<double&, double&, double&>())
        .def("fit", &Fit<double, double, double>::fit);

    pybind11::class_<Fit<double, double>>(m, "Fit")
        .def(pybind11::init<double&, double&>())
        .def("fit", &Fit<double, double>::fit);
    */
}