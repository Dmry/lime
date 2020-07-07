#pragma once

#include "../time_series.hpp"

#include "constraint_release.hpp"

#include <random>
#include <stdexcept>

struct LM_constraint_release : public IConstraint_release
{
  public:
    LM_constraint_release(Time_series::time_type time_range_, double c_v_, size_t realizations_);
    void update(const Context& ctx) override;
    virtual Time_series_functional::functional_type time_functional(const Context& ctx) override;

  private:
    double cp(double Gf_norm, double tau_df, double tau_e, double p_star, double e_star, double epsilon, double e_start);
    void generate(double Gf_norm, double tau_df, double tau_e, double e_star, double Z);
    double cp_one(double Gf_norm, double tau_df, double p_star, double epsilon);
    double cp_two(double Z, double tau_e, double e_star, double epsilon);
    double e_star(double Z, double tau_e, double G_f_normed);

    double Me(double epsilon);
    double integral_result(double t);
    auto R_t_functional(double Z, double tau_e, double G_f_normed, double tau_df);

    std::exception_ptr ep;

    size_t realizations_;
    std::vector<double> km;
    const std::seed_seq seed;
    std::mt19937  prng;
    std::uniform_real_distribution<double> dist;
};