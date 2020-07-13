#pragma once

#include <boost/iterator/zip_iterator.hpp>

#include "context.hpp"
#include "utilities.hpp"
#include "parallel_policy.hpp"

#include <vector>
#include <memory>
#include <functional>

struct Time_range
{
    using primitive = double;
    using base = std::vector<primitive>;
    using type = std::shared_ptr<base>;

    static Time_range::type generate_exponential(primitive base, primitive max)
    {
        Time_range::type time = Time_range::construct(static_cast<size_t>(log_with_base<primitive>(base, max)));

        std::generate(time->begin(), time->end(), [n=0, base] () mutable {return std::pow(base, n++);});

        return time;
    }

    static Time_range::type generate_normalized_exponential(primitive base, primitive max, primitive norm)
    {
        auto&& range = generate_exponential(base, max);

        std::for_each(exec_policy, range->begin(), range->end(), [norm](primitive& t){t /= norm;});

        return std::forward<type>(range);
    }

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

    Time_series(time_type time_range, value_type values)
    :   time_range_{time_range},
        values_(values)
    {
        if (time_range_->size() != values_.size())
            throw std::runtime_error("Tried to create a time series with incompatible time and value series.");
    }

    Time_series(const Time_series& other_time_series)
    :   time_range_{other_time_series.time_range_},
        values_(other_time_series.values_)
    {}

    auto time_zipped_begin() {return boost::make_zip_iterator(boost::make_tuple(time_range_->begin(), values_.begin()));}
    auto time_zipped_end()   {return boost::make_zip_iterator(boost::make_tuple(time_range_->end(), values_.end()));}

    std::vector<value_primitive> operator()() {return values_;}
    Time_series& operator=(const Time_series& other_time_series) noexcept {values_ = other_time_series.values_; time_range_=other_time_series.time_range_; return *this;}

    std::vector<value_primitive>::iterator begin() {return values_.begin();}
    std::vector<value_primitive>::iterator end() {return values_.end();}

    std::vector<value_primitive>::const_iterator cbegin() const {return values_.cbegin();}
    std::vector<value_primitive>::const_iterator cend() const {return values_.cend();}

    size_t size() const {return values_.size();}

    Time_series::time_type get_time_range() const {return time_range_;}
    Time_series::value_type get_values() const {return values_;}

    protected:
        Time_series::time_type time_range_;
        Time_series::value_type values_;
};

struct Time_functor
{
    using function_t = std::function<Time_series::value_primitive(Time_series::time_primitive)>;

    Time_functor()
    {}

    virtual Time_series operator()(const Time_series::time_type&) const = 0;
    virtual Time_series::value_primitive operator()(const Time_series::time_primitive&) const = 0;
};

struct Time_series_functional : public Time_series
{
    Time_series_functional(Time_series::time_type time_range)
    :   Time_series(time_range)
    {}
    
    using functional_type = std::function<Time_series::value_primitive (Time_series::time_primitive)>;

    // Provides a generalized interface for all time functionals
    virtual functional_type time_functional(const Context&) = 0;
};