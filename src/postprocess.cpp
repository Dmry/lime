#include <boost/math/differentiation/finite_difference.hpp>

#include "postprocess.hpp"

#include <algorithm>

Time_series derivative(const Time_functor& func, const Time_series::time_type& time_range)
{
    using namespace boost::math::differentiation;

    Time_series result(time_range);

    std::transform(exec_policy, time_range->begin(), time_range->end(), result.begin(), [&func](const Time_series::time_primitive& t)
    {
        return t;//finite_difference_derivative<decltype(func), Time_series::time_primitive, 8>(func, t);
    });

    return result;
}