#include "time_series.hpp"

#include <algorithm>
#include <cmath>
#include <ostream>

Time_range::type Time_range::generate_exponential(primitive base, primitive max)
{
    Time_range::type time = Time_range::construct(static_cast<size_t>(log_with_base<primitive>(base, max)));
    std::generate(time->begin(), time->end(), [n=0, base] () mutable {return std::pow(base, n++);});
    return time;
}

Time_range::type Time_range::generate_normalized_exponential(primitive base, primitive max, primitive norm)
{
    auto&& range = generate_exponential(base, max);

    std::for_each(exec_policy, range->begin(), range->end(), [norm](primitive& t){t /= norm;});

    return std::forward<type>(range);
}

Time_series::Time_series(time_type time_range)
:   time_range_{time_range},
    values_(time_range_->size())
{}

Time_series::Time_series(time_type time_range, value_type values)
:   time_range_{time_range},
    values_(values)
{
    if (time_range_->size() != values_.size())
        throw std::runtime_error("Tried to create a time series with incompatible time and value series.");
}

Time_series::Time_series(const Time_series& other_time_series)
:   time_range_{other_time_series.time_range_},
    values_(other_time_series.values_)
{}

using namespace boost::iterators;
using namespace boost::tuples;

zip_iterator<tuple<Time_range::base::iterator, Time_series::value_type::iterator>> Time_series::time_zipped_begin()
{
    return boost::make_zip_iterator(boost::make_tuple(time_range_->begin(), values_.begin()));
}

zip_iterator<tuple<Time_range::base::iterator, Time_series::value_type::iterator>> Time_series::time_zipped_end()
{
    return boost::make_zip_iterator(boost::make_tuple(time_range_->end(), values_.end()));
}

zip_iterator<tuple<Time_range::base::const_iterator, Time_series::value_type::const_iterator>> Time_series::time_zipped_begin() const
{
    return boost::make_zip_iterator(boost::make_tuple(time_range_->begin(), values_.begin()));
}

zip_iterator<tuple<Time_range::base::const_iterator, Time_series::value_type::const_iterator>> Time_series::time_zipped_end() const
{
    return boost::make_zip_iterator(boost::make_tuple(time_range_->end(), values_.end()));
}

std::vector<Time_series::value_primitive> Time_series::operator()()
{
    return values_;
}

Time_series& Time_series::operator=(const Time_series& other_time_series) noexcept
{
    values_ = other_time_series.values_;
    time_range_=other_time_series.time_range_;
    return *this;
}

std::vector<Time_series::value_primitive>::iterator Time_series::begin()
{
    return values_.begin();
}

std::vector<Time_series::value_primitive>::iterator Time_series::end()
{
    return values_.end();
}

std::vector<Time_series::value_primitive>::const_iterator Time_series::cbegin() const
{
    return values_.cbegin();
}

std::vector<Time_series::value_primitive>::const_iterator Time_series::cend() const
{
    return values_.cend();
}

size_t Time_series::size() const
{
    return values_.size();
}

Time_series::time_type Time_series::get_time_range() const
{
    return time_range_;
}

Time_series::value_type Time_series::get_values() const
{
    return values_;
}

std::ostream& operator<< (std::ostream &stream, const Time_series& series)
{
    std::ostream_iterator<boost::tuples::tuple<Time_series::time_primitive, Time_series::value_primitive>> out_it (stream,"\n");
    std::copy(series.time_zipped_begin(), series.time_zipped_end(), out_it);
 
    return stream;
}
