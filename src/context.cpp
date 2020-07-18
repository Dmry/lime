#include <boost/fusion/include/for_each.hpp>
#include <boost/fusion/include/algorithm.hpp>
#include <boost/phoenix/phoenix.hpp>
#include <boost/mpl/range_c.hpp>

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

std::ostream& operator<< (std::ostream &stream, const Context& context)
{
    using namespace boost::fusion;

    for_each(boost::mpl::range_c <
        unsigned, 0, result_of::size<Context>::value>(),
            [&](auto index) constexpr {
                stream << extension::struct_member_name<Context,index>::call() << " " << at_c<index>(context) << "\n";
            }
    );
 
    return stream;
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

BOOST_FUSION_ADAPT_STRUCT_NAMED(
    Context, ICS_context_struct,
    Z,
    M_e,
    N,
    N_e,
    G_f_normed,
    tau_e,
    tau_monomer,
    G_e,
    tau_r,
    tau_d_0,
    tau_df
)

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

void
ICS_context_builder::initialize()
{
    context_->apply_physics();
}

void
ICS_context_builder::validate_state()
{
    using namespace boost::fusion::adapted;
    using namespace checks;
    using namespace checks::policies;

    static std::string location("in ICS context builder");

    try
    {
        check<ICS_context_struct, is_nan<throws>, zero<prints_error_append<location>>>(*context_);
    }
    catch (const std::exception& ex)
    {
        std::throw_with_nested(std::runtime_error(location));
    }
}

Reproduction_context_builder::Reproduction_context_builder()
:   IContext_builder{}
{}

Reproduction_context_builder::Reproduction_context_builder(std::shared_ptr<Context> context)
:   IContext_builder{context}
{}

BOOST_FUSION_ADAPT_STRUCT_NAMED(
    Context, Reproduction_context_struct,
    Z,
    tau_e,
    N,
    G_f_normed,
    tau_d_0,
    tau_df
);

void
Reproduction_context_builder::gather_physics()
{
    context_->add_physics_in_place<G_f_normed>();
    context_->add_physics_in_place<Tau_d_0>();
    context_->add_physics_in_place<Tau_df>();
}

void
Reproduction_context_builder::initialize()
{
    context_->apply_physics();
}

void
Reproduction_context_builder::validate_state()
{
    using namespace boost::fusion::adapted;
    using namespace checks;
    using namespace checks::policies;

    try
    {
        check<ICS_context_struct, is_nan<throws>, zero<throws>>(*context_);
    }
    catch (const std::exception& ex)
    {
        std::throw_with_nested(std::runtime_error("in ICS context builder"));
    }
}