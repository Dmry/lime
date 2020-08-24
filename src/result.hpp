#pragma once

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

#include "context.hpp"
#include "time_series.hpp"
#include "contour_length_fluctuations.hpp"
#include "constraint_release/constraint_release.hpp"
#include "longitudinal_motion.hpp"
#include "rouse_motion.hpp"

#include <memory>
#include <vector>

class IResult : public Time_series
{
    public:
        explicit IResult(Time_series::time_type time_range);
        virtual void calculate() = 0;
};

struct ICS_result : public IResult
{
    ICS_result(Time_series::time_type time_range, IContext_builder* builder, constraint_release::impl impl, bool cr_observes_context = true);

    void calculate() override;

    Rouse_motion get_rouse_motion() const;
    Longitudinal_motion get_longitudinal_motion() const;

    std::shared_ptr<Context> context_;
    std::unique_ptr<Contour_length_fluctuations> CLF;
    std::unique_ptr<IConstraint_release> CR;
};

struct Derivative_result : public IResult
{
    Derivative_result(Time_series::time_type time_range, IContext_builder* builder, const Time_functor& func);

    void calculate() override;

    std::shared_ptr<Context> context_;
    const Time_functor& function_;
};