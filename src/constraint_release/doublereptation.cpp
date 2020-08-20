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

#include "doublereptation.hpp"

DR_constraint_release::DR_constraint_release(double c_v, Context &ctx)
    : IConstraint_release{c_v}, clf{ctx}
{
}

Time_series DR_constraint_release::operator()(const Time_series::time_type &time) const
{
    return clf(time);
}

Time_series::value_primitive DR_constraint_release::operator()(const Time_series::time_primitive &time) const
{
    return clf(time);
}

void DR_constraint_release::update(const Context &ctx)
{
    clf.update(ctx);
}

void DR_constraint_release::validate_update(const Context &) const
{
    // Handled by CLF itself
    return;
}