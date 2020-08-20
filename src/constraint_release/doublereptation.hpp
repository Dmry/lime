/*
 *  This file is part of Lime, a tool for the application of Likhtman & McLiesh' model for polymer dyanmics.
 *
 *  Copyright © 2020 Daniel Emmery (CNRS)
 *
 *  File contents:
 * 
 *  Allow contour length fluctuations to be used as an approximation for constraint release.
 *
 *  Not the most elegant solution as it leads to double computations, but definitely the most ergonmic one
 * 
 *  GPL 3.0 License
 * 
 */

#include "constraint_release.hpp"
#include "../contour_length_fluctuations.hpp"
#include "../context.hpp"

struct DR_constraint_release : public IConstraint_release
{
    DR_constraint_release(double c_v, Context &ctx);
    virtual ~DR_constraint_release() = default;

    Time_series operator()(const Time_series::time_type &) const override;
    Time_series::value_primitive operator()(const Time_series::time_primitive &) const override;

    void update(const Context &ctx) override;
    void validate_update(const Context &ctx) const override;

    private:
        Contour_length_fluctuations clf;
};