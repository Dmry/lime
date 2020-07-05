#include <boost/fusion/include/for_each.hpp>
#include <boost/fusion/include/algorithm.hpp>
#include <boost/phoenix/phoenix.hpp>
#include <boost/mpl/range_c.hpp>

#include "context.hpp"
#include "utilities.hpp"
#include "tube.hpp"

#include <iostream>
#include <stdexcept>

void Context::add_physics(Physics_ptr physics)
{
    physics_.emplace_back(std::move(physics));
}

void Context::apply_physics()
{
    for (auto& calculation : physics_)
        calculation->apply(*this);

    notify_computes();
}

void Context::attach_compute(Compute* compute)
{
    computes_.emplace_back(compute);
}

void Context::attach_compute(std::vector<Compute*> computes)
{
    using std::begin, std::end;
    computes_.insert(end(computes_), begin(computes), end(computes));
}

void Context::notify_computes() 
{
    for (auto compute : computes_)
    {
        if (compute != nullptr)
        {
            compute->update(*this);
        }
    }
}

struct check_nan_impl
{
    template<typename T>
    void operator()(T& t) const
    {
        if (t != t)
        {
            throw std::runtime_error("Got NaN");
        }
    }
};

// Checks if any of the values in the context is NaN
void Context::check_nan()
{
    using boost::phoenix::arg_names::arg1;

    boost::fusion::for_each(*this, check_nan_impl());
}

// Prints name of every variable in the context and its current value
// Logs to info if > 0, logs to warning if < 0
void Context::print()
{
    // Credit: Joao Tavora
    boost::fusion::for_each(boost::mpl::range_c<
        unsigned, 0, boost::fusion::result_of::size<Context>::value>(),
            [&](auto index){
                info_or_warn(boost::fusion::extension::struct_member_name<Context,index>::call(), boost::fusion::at_c<index>(*this));
            }
    );
}

IContext_builder::IContext_builder()
:   context_{std::make_shared<Context>()}
{}

IContext_builder::IContext_builder(std::shared_ptr<Context> context)
:   context_{context}
{}

void
IContext_builder::set_context(std::shared_ptr<Context> context)
{
    context_ = context;
}

std::shared_ptr<Context>
IContext_builder::get_context()
{
    return context_;
}

ICS_context_builder::ICS_context_builder(std::shared_ptr<struct System> system)
:   IContext_builder{}, system_{system}
{}

ICS_context_builder::ICS_context_builder(std::shared_ptr<struct System> system, std::shared_ptr<Context> context)
:   IContext_builder{context}, system_{system}
{}

void
ICS_context_builder::gather_physics()
{
    // Tube-related physics
    context_->add_physics_in_place<Z_from_length>();
    context_->add_physics_in_place<M_e_from_N_e>();
    context_->add_physics_in_place<G_f_normed>();

    // System-related physics
    context_->add_physics_in_place<Tau_e_alt>(system_);
    context_->add_physics_in_place<Tau_r>(system_);
    context_->add_physics_in_place<Tau_d_0>(system_);
    context_->add_physics_in_place<Tau_df>(system_);
    context_->add_physics_in_place<G_e>(system_);
}

void
ICS_context_builder::attach_computes(std::vector<Compute*> computes)
{
    context_->attach_compute(computes);
}