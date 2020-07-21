#pragma once

#include "context.hpp"
#include "time_series.hpp"
#include "contour_length_fluctuations.hpp"
#include "constraint_release/constraint_release.hpp"

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
    ICS_result(Time_series::time_type time_range, IContext_builder* builder, constraint_release::impl impl = constraint_release::impl::HEUZEY);

    void calculate() override;

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