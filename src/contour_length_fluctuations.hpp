#pragma once

#include "time_series.hpp"
#include "context.hpp"

#include <memory>
#include <vector>

// Iterators and data stroage inherited from Time_series, Compute allows non-owning coupling with a Context
struct Contour_length_fluctuations : public Time_series, public Compute 
{ 
   public:
    Contour_length_fluctuations(Time_series::time_type time_range_);
    Contour_length_fluctuations(const Contour_length_fluctuations&) = default;

    // Lower bound for integration
    static double e_star(double Z, double tau_e, double Gf_normed);

    // Returns an expression for survival probability mu(t) to be used in std::algorithms
    static auto mu_t_functional(double Z, double tau_e, double G_f_normed, double tau_df);

    // Calcualte new values for mu_t;
    void update(const Context& ctx) override;

  private:
    static double integral_result(double lower_bound, double t);
};