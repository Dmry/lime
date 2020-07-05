#include "result.hpp"
#include "longitudinal_motion.hpp"
#include "rouse_motion.hpp"
#include "contour_length_fluctuations.hpp"
#include "constraint_release/likhtmanmcleish.hpp"
#include "utilities.hpp"
#include "parallel_policy.hpp"

#include <algorithm>

IResult::IResult(Time_range::type time_range)
:   time_range_{time_range},
    result_buffer_(time_range_->size())
{}

ICS_result::ICS_result(Time_range::type time_range, IContext_builder* builder)
:   IResult(time_range)
{
    // Build context
    builder->gather_physics();
    CLF = std::make_unique<Contour_length_fluctuations>(time_range_);
    CR = std::make_unique<HEU_constraint_release>(time_range_, 0.1);

    builder->attach_computes({CLF.get(), CR.get()});

    context_ = builder->get_context();
}

double&
ICS_result::get_cv()
{
    return CR->c_v_;
}

std::vector<double>
ICS_result::result()
{
    if (!context_)
    {
        throw "No context for result!";
    }

    Longitudinal_motion LM{context_->Z, context_->tau_r};
    Rouse_motion RM{context_->Z, context_->tau_r, context_->N};

    std::transform(exec_policy, CLF->begin(), CLF->end(), CR->begin(), result_buffer_.begin(),
        [](const double& mu_t, const double& r_t){ return 4.0/5.0 * mu_t * r_t;}
    );

    std::transform(exec_policy, time_range_->begin(), time_range_->end(), result_buffer_.begin(), result_buffer_.begin(),
        [this, &LM, &RM](const double& t, const double& res){ return context_->G_e* (res  + LM(t) + RM(t) );}
    );

    return result_buffer_;
}