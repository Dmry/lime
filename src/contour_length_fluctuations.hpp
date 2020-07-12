#pragma once

#include "time_series.hpp"
#include "context.hpp"

#include <memory>
#include <vector>
#include <functional>

struct Contour_length_fluctuations : public Time_functor, Compute
{ 
   public:
    // Get state of, but don't attach to a context
    Contour_length_fluctuations(double Z, double tau_e, double G_f_normed, double tau_df);
    // Get state of, and attach to a context
    explicit Contour_length_fluctuations(Context& ctx);
    Contour_length_fluctuations(const Contour_length_fluctuations&) = default;

    // Lower bound for integration
    static double e_star(double Z, double tau_e, double G_f_normeded);

    Time_series operator()(const Time_series::time_type&) override;
    Time_series::value_primitive operator()(const Time_series::time_primitive&) override;

    // Calcualte new values for mu_t;
    void update(const Context& ctx) override;

  private:
    static double integral_result(double lower_bound, double t);

    double Z_;
    double tau_e_;
    double G_f_normed_;
    double tau_df_;
    double p_star_;
    double e_star_;
};