#include "clf.hpp"

/*
    Not the most elegant solution as it leads to double computations, but definitely the most ergonmic one
*/

CLF_constraint_release::CLF_constraint_release(double c_v, Context &ctx)
    : IConstraint_release{c_v}, clf{ctx}
{
}

Time_series CLF_constraint_release::operator()(const Time_series::time_type &time) const
{
    return clf(time);
}

Time_series::value_primitive CLF_constraint_release::operator()(const Time_series::time_primitive &time) const
{
    return clf(time);
}

void CLF_constraint_release::update(const Context &ctx)
{
    clf.update(ctx);
}

void CLF_constraint_release::validate_update(const Context &) const
{
    // Handled by CLF itself
    return;
}