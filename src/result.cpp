/*
 *  This file is part of Lime, a tool for the application of Likhtman & McLiesh' model for polymer dyanmics.
 *
 *  Copyright © 2020 Daniel Emmery (CNRS)
 *
 *  File contents:
 * 
 *  Datatypes for computation of model results
 *
 *  GPL 3.0 License
 * 
 */

#include "result.hpp"
#include "contour_length_fluctuations.hpp"
#include "utilities.hpp"
#include "parallel_policy.hpp"
#include "postprocess.hpp"

#include <algorithm>

IResult::IResult(Time_series::time_type time_range)
:   Time_series{time_range}
{}

ICS_result::ICS_result(Time_series::time_type time_range, IContext_builder* builder, constraint_release::impl impl, bool cr_observes_context)
:   IResult(time_range)
{
    // Build context
    builder->gather_physics();
    builder->initialize();
    builder->validate_state();
    context_ = builder->get_context();

    if (cr_observes_context)
    {
        CR = constraint_release::Factory_observed::create(impl, 0.1, *context_);
    }
    else
    {
        CR = constraint_release::Factory::create(impl, 0.1, context_->Z, context_->tau_e, context_->G_f_normed, context_->tau_df);
    }

    CLF = std::make_unique<Contour_length_fluctuations>(*context_);
}

void
ICS_result::calculate()
{
    if (!context_)
    {
        throw std::runtime_error("No context for ICS result!");
    }

    Longitudinal_motion LM = get_longitudinal_motion();
    Rouse_motion RM = get_rouse_motion();

    std::transform(std::execution::seq, time_range_->begin(), time_range_->end(), values_.begin(),
        [this, &LM, &RM](const double& t){ 
        return context_->G_e* (4.0/5.0 * (*CR)(t) * (*CLF)(t) + LM(t) + RM(t) );}
    );
}

Rouse_motion
ICS_result::get_rouse_motion() const
{
    return Rouse_motion{context_->Z, context_->tau_r, context_->N};
}

Longitudinal_motion
ICS_result::get_longitudinal_motion() const
{
    return Longitudinal_motion{context_->Z, context_->tau_r};
}

Derivative_result::Derivative_result(Time_series::time_type time_range, IContext_builder *builder, const Time_functor &func)
    : IResult{time_range}, function_{func}
{
    builder->gather_physics();
    builder->initialize();
    builder->validate_state();
    context_ = builder->get_context();
}

void
Derivative_result::calculate()
{
    if (!context_)
    {
        throw "No context for derivative result!";
    }

    auto wrapper = [this](const double& t) {return (function_)(t);};

    dynamic_cast<Time_series&>(*this) = dimensionless_derivative(wrapper, time_range_, context_);
}