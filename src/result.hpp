#pragma once

#include "context.hpp"
#include "tube.hpp"
#include "contour_length_fluctuations.hpp"
#include "constraint_release/heuzley.hpp"
#include "constraint_release/constraint_release.hpp"

#include <memory>
#include <vector>

class IResult
{
    public:
        IResult(Time_range::type time_range);
        virtual std::vector<double> result() = 0;

    protected:
        Time_range::type time_range_;
        std::vector<double> result_buffer_;
};

class ICS_result : public IResult
{
    public:
        ICS_result(Time_range::type time_range, IContext_builder* builder);

        std::vector<double> result() override;

        void set_c_v(double);

        std::shared_ptr<Context> context_;

    private:
        std::unique_ptr<Contour_length_fluctuations> CLF;
        std::unique_ptr<IConstraint_release> CR;
};