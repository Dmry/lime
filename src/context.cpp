/*
 *  This file is part of Lime, a tool for the application of Likhtman & McLiesh' model for polymer dyanmics.
 *
 *  Copyright © 2020 Daniel Emmery (CNRS)
 *
 *  File contents:
 * 
 *  Data types for model parameters
 *
 *  GPL 3.0 License
 * 
 */

#include "context.hpp"
#include "utilities.hpp"
#include "tube.hpp"
#include "checks.hpp"

#include <iostream>
#include <stdexcept>

void Context::add_physics(Physics_ptr physics)
{
    physics_.emplace_back(std::move(physics));
}

void Context::apply_physics()
{
    for (auto& calculation : physics_)
    {
        if(calculation != nullptr)
        {
            calculation->apply(*this);
        }
        else
        {
            BOOST_LOG_TRIVIAL(warning) << "Tried to execute expired physics. Results may be incomplete.";
        }
    }

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
        else
        {
            BOOST_LOG_TRIVIAL(warning) << "Tried to execute expired compute. Results may be incomplete.";
        }
    }
}

std::ostream& operator<<(std::ostream& stream, const IContext_view& view)
{
    return view.serialize(stream);
}

IContext_builder::IContext_builder()
:   context_{std::make_shared<Context>()}
{}

IContext_builder::IContext_builder(std::shared_ptr<Context> context)
:   context_{context}
{}

IContext_builder::~IContext_builder()
{};

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
    context_->add_physics_in_place<G_e>(system_);

    // Tau_e dependent physics
    context_->add_physics_in_place<Tau_r>();
    context_->add_physics_in_place<Tau_d_0>();
    context_->add_physics_in_place<Tau_df>();
}

constexpr auto ICS_keys = boost::hana::make_tuple(
    LIME_KEY(Z),
    LIME_KEY(M_e),
    LIME_KEY(N),
    LIME_KEY(N_e),
    LIME_KEY(G_f_normed),
    LIME_KEY(tau_e),
    LIME_KEY(tau_monomer),
    LIME_KEY(G_e),
    LIME_KEY(tau_r),
    LIME_KEY(tau_d_0),
    LIME_KEY(tau_df));

void
ICS_context_builder::initialize()
{
    context_->apply_physics();
}

void
ICS_context_builder::validate_state()
{
    using namespace checks;
    using namespace checks::policies;

    static const std::string location("in ICS context builder");

    try
    {
        check<is_nan<throws>, zero<prints_warning_append<location>>>(this->context_view().get());
    }
    catch (const std::exception&)
    {
        std::throw_with_nested(std::runtime_error(location));
    }
}

std::unique_ptr<IContext_view>
ICS_context_builder::context_view()
{
    return std::make_unique<Context_view<ICS_keys>>(*context_);
}

std::shared_ptr<System>
ICS_context_builder::get_system()
{
    return system_;
}

ICS_decoupled_context_builder::ICS_decoupled_context_builder(std::shared_ptr<struct System> system)
    : ICS_context_builder{system}
{
}

ICS_decoupled_context_builder::ICS_decoupled_context_builder(std::shared_ptr<struct System> system, std::shared_ptr<Context> context)
    : ICS_context_builder{system, context}
{
}

void ICS_decoupled_context_builder::gather_physics()
{
    // Tube-related physics
    context_->add_physics_in_place<Z_from_length>();
    context_->add_physics_in_place<M_e_from_N_e>();
    context_->add_physics_in_place<G_f_normed>();

    // System-related physics
    context_->add_physics_in_place<Tau_e_alt>(system_);

    // Tau_e dependent physics
    context_->add_physics_in_place<Tau_r>();
    context_->add_physics_in_place<Tau_d_0>();
    context_->add_physics_in_place<Tau_df>();
}

Reproduction_context_builder::Reproduction_context_builder()
:   IContext_builder{}
{}

Reproduction_context_builder::Reproduction_context_builder(std::shared_ptr<Context> context)
:   IContext_builder{context}
{}

void
Reproduction_context_builder::gather_physics()
{
    context_->add_physics_in_place<G_f_normed>();
    context_->add_physics_in_place<Tau_d_0>();
    context_->add_physics_in_place<Tau_df>();
}

constexpr auto Reproduction_keys = boost::hana::make_tuple(
    LIME_KEY(Z),
    LIME_KEY(G_f_normed),
    LIME_KEY(tau_e),
    LIME_KEY(tau_d_0),
    LIME_KEY(tau_df));

void
Reproduction_context_builder::initialize()
{
    context_->apply_physics();
}

void
Reproduction_context_builder::validate_state()
{
    using namespace checks;
    using namespace checks::policies;

    try
    {
        check<is_nan<throws>, zero<throws>>(this->context_view().get());
    }
    catch (const std::exception&)
    {
        std::throw_with_nested(std::runtime_error("in reproduction context builder"));
    }
}

std::unique_ptr<IContext_view>
Reproduction_context_builder::context_view()
{
    return std::make_unique<Context_view<Reproduction_keys>>(*context_);
}

IContext_view::IContext_view(const Context &ctx)
: context_{ctx}
{}
