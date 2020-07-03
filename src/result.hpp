#pragma once

#include "context.hpp"
#include "tube.hpp"
#include "system.hpp"
#include "contour_length_fluctuations.hpp"
#include "HEU_constraint_release.hpp"
#include "Iconstraint_release.hpp"

#include <memory>
#include <vector>

class IResult
{
    public:
        IResult(Time_range::type time_range);
        virtual std::vector<double> result() = 0;

    protected:
        Time_range::type time_range_;
        std::vector<double> result_buffer;
};

class ICS_result : public IResult
{
    public:
        ICS_result(Time_range::type time_range, ICS_context_builder* builder);

        std::vector<double> result() override;

        double c_v;

    private:
        std::shared_ptr<Context> context_;

        std::unique_ptr<Contour_length_fluctuations> CLF;
        std::unique_ptr<IConstraint_release> CR;
};