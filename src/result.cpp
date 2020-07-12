#include "result.hpp"
#include "longitudinal_motion.hpp"
#include "rouse_motion.hpp"
#include "contour_length_fluctuations.hpp"
#include "utilities.hpp"
#include "parallel_policy.hpp"

#include <algorithm>

IResult::IResult(Time_series::time_type time_range)
:   Time_series{time_range}
{}

ICS_result::ICS_result(Time_series::time_type time_range, IContext_builder* builder, constraint_release::impl impl)
:   IResult(time_range)
{
    // Build context
    builder->gather_physics();
    context_ = builder->get_context();

    CR = constraint_release::Factory_with_context::create(impl, 0.1, *context_);
    CLF = std::make_unique<Contour_length_fluctuations>(*context_);
}

Time_series::value_type
ICS_result::result()
{
    if (!context_)
    {
        throw "No context for result!";
    }

    Longitudinal_motion LM{context_->Z, context_->tau_r};
    Rouse_motion RM{context_->Z, context_->tau_r, context_->N};

    std::transform(exec_policy, time_range_->begin(), time_range_->end(), values_.begin(),
        [this, &LM, &RM](const double& t){ return context_->G_e* (4.0/5.0 * (*CR)(t) * (*CLF)(t) + LM(t) + RM(t) );}
    );

    return values_;
}