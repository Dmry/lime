#pragma once

/*
 *      Theory for Linear Rheology of Monodisperse Linear Polymers
 *      Marie-Claude Heuzey, Paula Wood-Adams, Djamila Sekki
 *      Journal of Applied Polymer Science 2004
 */

#include "../utilities.hpp"
#include "constraint_release.hpp"

#include <vector>
#include <variant>
#include <mutex>
#include <stdexcept>

namespace constraint_release
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
    virtual double operator () (double epsilon) const override;

  protected:
    
    double low(double epsilon) const;
    double high(double epsilon) const;
};

// Equation 17
struct Medium : public Short
{
    Medium(const double Z_, const double tau_e_, const double tau_df_);

    virtual double operator () (double epsilon) const override;

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

    virtual double operator () (double epsilon) const override;

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
    struct userdata {
        std::unique_ptr<constraint_release::IModel> model;
        double t;
    };

  protected:

    double epsilon_0;
    std::unique_ptr<constraint_release::IModel> model;

  public:

    HEU_constraint_release(Time_range::type time_range, double c_v);

    auto R_t_functional(double Z, double tau_e);
    void update(const Context& ctx) override;

  private:
    std::mutex mtx;
    std::exception_ptr ep;

    // Equation 20, select lower bound for integration
    double epsilon_zero(double Z, double tau_e);
    double integral_result(double lower_bound, double t);
};