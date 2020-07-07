#include <boost/math/differentiation/finite_difference.hpp>

#include "postprocess.hpp"

#include <algorithm>

Time_series derivative(Time_series_functional::functional_type functional, Time_series_functional::time_type time_range)
{
    using namespace boost::math::differentiation;

    Time_series result(time_range);

    std::transform(exec_policy, time_range->begin(), time_range->end(), result.begin(), [&functional](const Time_series_functional::time_primitive& t)
    {
        return finite_difference_derivative<decltype(functional), Time_series_functional::time_primitive, 2>(functional, t);
    });

    return result;
}