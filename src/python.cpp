#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "context.hpp"
#include "tube.hpp"
#include "result.hpp"
#include "time_series.hpp"
#include "constraint_release/constraint_release.hpp"
#include "constraint_release/rubinsteincolby.hpp"
#include "constraint_release/heuzey.hpp"

using heu_factory = Register_class<IConstraint_release, HEU_constraint_release, constraint_release::impl, double, Context &>;

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

    pybind11::class_<ICS_context_builder>(m, "Context_builder")
        .def(pybind11::init<std::shared_ptr<System>, std::shared_ptr<Context>>());

    pybind11::enum_<constraint_release::impl>(m, "cr_impl")
        .value("Heuzey", constraint_release::impl::HEUZEY)
        .value("Rubinsteincolby", constraint_release::impl::RUBINSTEINCOLBY);

    m.def(
        "init_factories",
        []() { Register_class<IConstraint_release, HEU_constraint_release, constraint_release::impl, double, Context &> heu_factory(constraint_release::impl::HEUZEY);
               Register_class<IConstraint_release, HEU_constraint_release, constraint_release::impl, double, Context &> rub_factory(constraint_release::impl::RUBINSTEINCOLBY);
        },
        pybind11::return_value_policy::automatic);

    m.def("generate_exponential", &generate_exponential_wrapper, pybind11::return_value_policy::copy);

    pybind11::class_<ICS_result>(m, "ICS_result")
        .def(pybind11::init([](const Time_range::base& time, ICS_context_builder *builder, constraint_release::impl impl) {
            return ICS_result(std::make_shared<Time_range::base>(time), builder, impl);
        }))
        .def("calculate", &ICS_result::calculate)
        .def("get_values", &ICS_result::get_values);
}