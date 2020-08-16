#include "constraint_release.hpp"
#include "../contour_length_fluctuations.hpp"
#include "../context.hpp"

struct CLF_constraint_release : public IConstraint_release
{
    CLF_constraint_release(double c_v, Context &ctx);
    virtual ~CLF_constraint_release() = default;

    Time_series operator()(const Time_series::time_type &) const override;
    Time_series::value_primitive operator()(const Time_series::time_primitive &) const override;

    void update(const Context &ctx) override;
    void validate_update(const Context &ctx) const override;

    private:
        Contour_length_fluctuations clf;
};