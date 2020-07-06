#pragma once

#include <boost/iterator/zip_iterator.hpp>

#include <vector>
#include <memory>

struct Time_range
{
    using primitive = double;
    using base = std::vector<primitive>;
    using type = std::shared_ptr<base>;

    template<typename... T>
    static
    type
    construct(T&&... init)
    {
        return std::make_shared<base>(base((std::forward<T>(init), ...)));
    }

    static
    type
    convert(const base& init)
    {
        return std::make_shared<base>(init);
    }
};

struct Time_series
{
    using value_primitive = double;
    using value_type = std::vector<value_primitive>;

    using time_primitive = Time_range::primitive;
    using time_type = Time_range::type;

    Time_series(time_type time_range)
    :   time_range_{time_range},
        values_(time_range_->size())
    {}

    Time_series(const Time_series& other_time_series)
    :   time_range_{other_time_series.time_range_},
        values_(other_time_series.values_)
    {}

    auto time_zipped_begin() {return boost::make_zip_iterator(boost::make_tuple(time_range_->begin(), values_.begin()));}
    auto time_zipped_end()   {return boost::make_zip_iterator(boost::make_tuple(time_range_->end(), values_.end()));}

    std::vector<value_primitive> operator()() {return values_;}

    std::vector<value_primitive>::iterator begin() {return values_.begin();}
    std::vector<value_primitive>::iterator end() {return values_.end();}

    std::vector<value_primitive>::const_iterator cbegin() {return values_.cbegin();}
    std::vector<value_primitive>::const_iterator cend() {return values_.cend();}

    size_t size() {return values_.size();}

    protected:
        Time_series::time_type time_range_;
        Time_series::value_type values_;
};