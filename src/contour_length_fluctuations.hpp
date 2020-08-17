#pragma once

/*
 *  This file is part of Lime, a tool for the application of Likhtman & McLiesh' model for polymer dyanmics.
 *
 *  Copyright © 2020 Daniel Emmery (CNRS)
 *
 *  File contents:
 * 
 *  Computation of contour length fluctuations
 *
 *  GPL 3.0 License
 * 
 */

#include "time_series.hpp"
#include "context.hpp"
#include "checks.hpp"

#include <memory>
#include <vector>
#include <functional>

struct Contour_length_fluctuations : public Time_functor, Compute
{ 
    using sum_t = Summation<double, Time_series::time_primitive>;

   public:
    // Get state of, but don't attach to a context
    Contour_length_fluctuations(double Z, double tau_e, double G_f_normed, double tau_df);
    // Get state of, and attach to a context
    explicit Contour_length_fluctuations(Context& ctx);
    Contour_length_fluctuations(const Contour_length_fluctuations&) = default;

    Time_series operator()(const Time_series::time_type&) const override;
    Time_series::value_primitive operator()(const Time_series::time_primitive&) const override;

    // Calcualte new values for mu_t;
    void update(const Context& ctx) override;

  private:
    static double integral_result(double lower_bound, double t);
    // Lower bound for integration
    static double e_star(double Z, double tau_e, double G_f_normed);  
    sum_t sum_term(double Z, double tau_df);
    double norm(double Z, double tau_e);
    void validate_update(const Context& ctx) const override;

    double G_f_normed_;
    double e_star_;
    double norm_;
    sum_t sum_;
};
