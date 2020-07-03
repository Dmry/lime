#include "utilities.hpp"

Time_range::type
test_exponential_time_range()
{
    //Time_range::base time_range(100);
    //std::generate(time_range.begin(), time_range.end(), [n=1]() mutable {return n *= 1.2;});
    Time_range::base time_range{1.0, 2.0, 3.0, 4.0, 8.0, 16.0, 32.0, 64.0, 128.0, 256.0, 512.0};
    return Time_range::convert(time_range);
}

Time_range::type
test_time_range()
{
    return Time_range::construct(1.0, 2.0, 5.0, 10.0, 20.0, 40.0);
}