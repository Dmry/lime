#pragma once

#include "ics_log_utils.hpp"

#include <variant>
#include <cmath>
#include <stdexcept>
#include <functional>
#include <memory>
#include <vector>
#include <any>

static constexpr double pi = 3.14159265359;
static constexpr double gas_constant = 1; // m^2 kg s^-2 K^-1 mol^-1
static constexpr double k_B = 1;          // J K^-1
static constexpr double T_static = 1;     // K

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

struct log_with_base
{
    static double
    calculate(double base, double x)
    {
        return std::log(x)/std::log(base);
    }
};

/* template<typename... T>
void
check_nan_impl(T... x)
{
    if ( ((x != x), ...) )
    {
        throw std::runtime_error("Got NaN");
	}
}
 */
template<typename T>
struct sigmoid_wrapper
{
    T& value;

    sigmoid_wrapper(T& value_) : value{value_} {}

    operator T&() { return value; }
    operator T() const { return value; }

    T operator =(T value_) {return value = sigmoid(value_);}
    T operator =(T value_) const {return value = sigmoid(value_);}

    double
    sigmoid(double in)
    {
        return 1/(1+std::abs(in));
    }
};

template<typename T>
T
square(const T& param)
{
    return param*param;
}

template<typename T>
T typesafe_voidptr_cast(void* data)
{
    return std::any_cast<T>(*static_cast<std::any*>(data));
}

// Resolves variant based polymorphism
#pragma GCC diagnostic ignored "-Wreturn-type"
template<typename... T>
auto
resolve(std::variant<T...>& var)
{
    // guaranteed reached by the standard
    if (auto item = (std::get_if<T>(&var), ...))
    {
        return item;
    }
}

template<typename T>
void
info_or_warn(const std::string& description, T var)
{
    if (var < 0 or var != var)
    {
        BOOST_LOG_TRIVIAL(warning) << description << ": " << var;
    }
    else
    {
        BOOST_LOG_TRIVIAL(info) << description << ": " << var;
    }
}

template<typename T>
struct Summation
{
    using sum_func_t = std::function<T(const T&)>;

    Summation(T start_, T end_, T step_ = 1) : start{start_}, end{end_}, step{step_} {}

    T operator() (sum_func_t func)
    {
        T sum {0};
        T p {start};

        // Should really implement a comparison suitable for floating point numbers
        for(; p <= end; p += step)
            sum += func(p); 

        return sum;
    }

    T start;
    T end;
    T step;
};