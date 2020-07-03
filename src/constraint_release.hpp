#pragma once

struct Constraint_release
{
    using Model = std::variant<std::monostate,
                               constraint_release::Short,
                               constraint_release::Medium,
                               constraint_release::Long,
                               constraint_release::Extra_long>;

    struct userdata {
        Model model;
        double t;
    };

  protected:

    double epsilon_0;
    Model model;
    Time_range::type time_range;

  public:
    std::vector<double> R_t;
    double c_v;

    Constraint_release(double c_v_, Time_range::type time_range_);

    auto R_t_functional();
    void calculate(double Z_, double tau_e_, double tau_df_);

  private:

    // Equation 20, select lower bound for integration
    void set_epsilon();
    double integral_result(double t);

    double Z;
    double tau_e;
    double tau_df;

};  