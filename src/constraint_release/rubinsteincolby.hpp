#pragma once

#include "../time_series.hpp"

#include "constraint_release.hpp"
#ifndef CUDA
  #include "rubinsteincolby_cu.hpp"
#endif

#include <random>
#include <stdexcept>

struct RUB_constraint_release : public IConstraint_release
{
  public:
    RUB_constraint_release(double c_v, Context& ctx);
    RUB_constraint_release(double c_v, double Z, double tau_e, double G_f_normed, double tau_df);
    void update(const Context& ctx) override;
    Time_series operator()(const Time_series::time_type&) const override;
    Time_series::value_primitive operator()(const Time_series::time_primitive&) const override;

  private:
    double cp(double G_f_normed, double tau_df, double tau_e, double p_star, double e_star, double epsilon, double e_start);
    void generate(double G_f_normed, double tau_df, double tau_e, double e_star, double Z);
    double cp_one(double G_f_normed, double tau_df, double p_star, double epsilon);
    double cp_two(double Z, double tau_e, double e_star, double epsilon);
    double e_star(double Z, double tau_e, double G_f_normed);
    void set_sizing_requirements(size_t Z);
    void validate_update(const Context& ctx);

    double Me(double&& epsilon) const;
    double integral_result(double t) const;

    size_t realizations_;
    size_t realization_size_;
    
    #ifndef CUDA
    Cu_me_details cudetails;
    #endif

    std::vector<double> km;
    const std::seed_seq seed;
    std::mt19937  prng;
    std::uniform_real_distribution<double> dist;
};