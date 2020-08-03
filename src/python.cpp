#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "context.hpp"
#include "tube.hpp"
#include "result.hpp"
#include "time_series.hpp"
#include "constraint_release/constraint_release.hpp"

PYBIND11_MODULE(lime_python, m)
{
    pybind11::class_<System>(m, "System")
        .def_readwrite("temperature", &System::T)
        .def_readwrite("friction", &System::friction)
        .def_readwrite("density", &System::rho);

    pybind11::class_<Context>(m, "Context")
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
        .def(pybind11::init<std::shared_ptr<struct System>, std::shared_ptr<Context>>());

    pybind11::enum_<constraint_release::impl>(m, "cr_impl")
        .value("Heuzey", constraint_release::impl::HEUZEY)
        .value("Rubinsteincolby", constraint_release::impl::RUBINSTEINCOLBY);

   // m.def("generate_exponential", &Time_range::generate_exponential, pybind11::return_value_policy::copy);
   // m.def("time_range_from_vector", &Time_range::convert, pybind11::return_value_policy::copy);

/*     pybind11::class_<ICS_result>(m, "ICS_result")
        .def(pybind11::init<Time_series::time_type, ICS_context_builder*, constraint_release::impl>())
        .def("calculate", &ICS_result::calculate); */
}