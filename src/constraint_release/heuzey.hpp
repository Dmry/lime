#pragma once

/*
 *  This file is part of Lime, a tool for the application of Likhtman & McLiesh' model for polymer dyanmics.
 *
 *  Copyright © 2020 Daniel Emmery (CNRS)
 *
 *  File contents:
 * 
 *  Heuzey's method for constraint release
 *  Doi: 10.1002/app.20881
 *
 *  GPL 3.0 License
 * 
 */

#include "../time_series.hpp"
#include "../checks.hpp"
#include "../factory.hpp"

#include "constraint_release.hpp"

#include <vector>
#include <variant>
#include <mutex>
#include <stdexcept>

namespace heuzey_detail
{

struct IModel
{
    IModel(const double Z_, const double tau_e_, const double tau_df_);
    virtual double operator() (double epsilon) const = 0;

  protected:
    const double Z;
    const double tau_e;
    const double tau_df;

    double B1;
    double B2;
};

// Equation 15
struct Short : public IModel
{
    Short(const double Z_, const double tau_e_, const double tau_df_);
    double operator () (double epsilon) const override;

  protected:
    double low(double epsilon) const;
    double high(double epsilon) const;
};

// Equation 17
struct Medium : public Short
{
    Medium(const double Z_, const double tau_e_, const double tau_df_);

    double operator () (double epsilon) const override;

  protected:
    double n;
    double epsilon_B;

    double medium_low(double epsilon) const;
    double medium(double epsilon) const;
};

// Equation 19
struct Long : public Medium
{
  private:
    double epsilon_C;

  public:
    Long(const double Z_, const double tau_e_, const double tau_df_);

    double operator () (double epsilon) const override;

  protected:
    double medium_high(double epsilon) const;
};

struct Extra_long : public Long
{
    Extra_long(const double Z_, const double tau_e_, const double tau_df_);
};

}

struct HEU_constraint_release : public IConstraint_release
{
    using Model_ptr = std::unique_ptr<heuzey_detail::IModel>;

  public:
    HEU_constraint_release(double c_v, Context& ctx);
    HEU_constraint_release(double c_v, double Z, double tau_e, double tau_df);

    auto R_t_functional(double Z, double tau_e, double tau_df);

    Time_series operator()(const Time_series::time_type&) const override;
    Time_series::value_primitive operator()(const Time_series::time_primitive&) const override;
    void update(const Context& ctx) override;

  private:
    // Equation 20, select lower bound for integration
    double epsilon_zero(double Z, double tau_e) const;
    double integral_result(double lower_bound, double t) const;
    Model_ptr get_model(double Z, double tau_e, double tau_df);
    void validate_update(const Context& ctx) const override;

    Model_ptr model_;
    double epsilon_zero_;
    double tau_e_;
};