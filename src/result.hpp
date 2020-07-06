#pragma once

#include <boost/iterator/zip_iterator.hpp>

#include "context.hpp"
#include "time_series.hpp"
#include "tube.hpp"
#include "contour_length_fluctuations.hpp"
#include "constraint_release/heuzey.hpp"
#include "constraint_release/constraint_release.hpp"

#include <memory>
#include <vector>

class IResult : public Time_series
{
    public:
        IResult(Time_series::time_type time_range);
        virtual Time_series::value_type result() = 0;
};

struct ICS_result : public IResult
{
    ICS_result(Time_series::time_type time_range, IContext_builder* builder);

    Time_series::value_type result() override;

    std::shared_ptr<Context> context_;
    std::unique_ptr<Contour_length_fluctuations> CLF;
    std::unique_ptr<IConstraint_release> CR;
};