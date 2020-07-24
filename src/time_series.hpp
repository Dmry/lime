#pragma once

#include <boost/iterator/zip_iterator.hpp>
#include <boost/tuple/tuple_io.hpp>

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

    static Time_range::type generate_exponential(primitive base, primitive max);
    static Time_range::type generate_normalized_exponential(primitive base, primitive max, primitive norm);

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

    Time_series(time_type time_range);
    Time_series(time_type time_range, value_type values);
    Time_series(const Time_series& other_time_series);
    virtual ~Time_series() = default;

    boost::iterators::zip_iterator<boost::tuples::tuple<Time_range::base::iterator, Time_series::value_type::iterator>> time_zipped_begin();
    boost::iterators::zip_iterator<boost::tuples::tuple<Time_range::base::iterator, Time_series::value_type::iterator>> time_zipped_end();

    boost::iterators::zip_iterator<boost::tuples::tuple<Time_range::base::const_iterator, Time_series::value_type::const_iterator>>  time_zipped_begin() const;
    boost::iterators::zip_iterator<boost::tuples::tuple<Time_range::base::const_iterator, Time_series::value_type::const_iterator>>  time_zipped_end()   const;

    std::vector<value_primitive> operator()();
    Time_series& operator=(const Time_series& other_time_series) noexcept;

    std::vector<value_primitive>::iterator begin();
    std::vector<value_primitive>::iterator end();

    std::vector<value_primitive>::const_iterator cbegin() const;
    std::vector<value_primitive>::const_iterator cend() const;

    size_t size() const;

    friend std::ostream& operator<< (std::ostream& stream, const Time_series& series);

    Time_series::time_type get_time_range() const;
    Time_series::value_type get_values() const;

    protected:
        Time_series::time_type time_range_;
        Time_series::value_type values_;
};

struct Time_functor
{
    using function_t = std::function<Time_series::value_primitive(Time_series::time_primitive)>;

    virtual ~Time_functor() = default;

    virtual Time_series operator()(const Time_series::time_type&) const = 0;
    virtual Time_series::value_primitive operator()(const Time_series::time_primitive&) const = 0;
};
