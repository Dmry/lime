#include "result.hpp"
#include "longitudinal_motion.hpp"
#include "rouse_motion.hpp"
#include "contour_length_fluctuations.hpp"
#include "HEU_constraint_release.hpp"
#include "utilities.hpp"
#include "parallel_policy.hpp"

Result::Result()
:   context_{std::make_shared<Context>()},
    time_range_{},
    result_buffer(),
    CLF{},
    CR{}
{}


void Result::auto_configure(Result_builder* builder)
{
    builder_ = builder;

    // Build context
    builder_->gather_physics();
    context_ = builder_->get_context();

    // Allocate resources and attach computes
    builder_->allocate_result_buffer(result_buffer);
    time_range_ = builder_->get_time_range();
    CR = builder_->get_constraint_release();
    CLF = builder_->get_contour_length_fluctuations();
}

std::vector<double>
Result::result()
{
    if (!context_)
    {
        throw "No context for result!";
    }

    Longitudinal_motion LM{context_->Z, context_->tau_r};
    Rouse_motion RM{context_->Z, context_->tau_r, context_->N};

    std::transform(exec_policy, CLF->begin(), CLF->end(), CR->begin(), result_buffer.begin(),
        [](const double& mu_t, const double& r_t){ return 4.0/5.0 * mu_t * mu_t /*r_t*/;}
    );

    std::transform(exec_policy, time_range_->begin(), time_range_->end(), result_buffer.begin(), result_buffer.begin(),
        [this, &LM, &RM](const double& t, const double& res){ return context_->G_e* (res  + LM(t) + RM(t) );}
    );

    return result_buffer;
}

Result_builder::Result_builder(Time_range::type time_range)
:   context_{std::make_shared<Context>()},
    time_range_{time_range}
{}

void
Result_builder::set_context(std::shared_ptr<Context> ctx)
{
    context_ = ctx;
}

std::shared_ptr<Context>
Result_builder::get_context()
{
    if (!context_)
        context_ = std::make_shared<Context>();
    
    return context_;
}

Default_result_builder::Default_result_builder(std::shared_ptr<System> sys, Time_range::type time_range, double c_v)
:   Result_builder{time_range},
    c_v_{c_v},
    system_{sys}
{}

void
Default_result_builder::gather_physics()
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
Default_result_builder::allocate_result_buffer(std::vector<double>& buf)
{
    buf.resize(time_range_->size());
}

Time_range::type
Default_result_builder::get_time_range()
{
    return time_range_;
}

std::unique_ptr<IConstraint_release>
Default_result_builder::get_constraint_release()
{
    std::unique_ptr<IConstraint_release> CR = std::make_unique<HEU_constraint_release>(time_range_, c_v_);

    context_->attach_compute(CR.get());

    return CR;
}

std::unique_ptr<Contour_length_fluctuations>
Default_result_builder::get_contour_length_fluctuations()
{
    std::unique_ptr<Contour_length_fluctuations> CLF =  std::make_unique<Contour_length_fluctuations>(time_range_);

    context_->attach_compute(CLF.get());

    return CLF;
}