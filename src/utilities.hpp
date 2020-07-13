#pragma once

#include "lime_log_utils.hpp"

#include <cmath>
#include <stdexcept>
#include <functional>
#include <memory>
#include <any>
#include <ostream>

static constexpr double pi = 3.14159265359;
static constexpr double gas_constant = 1; // m^2 kg s^-2 K^-1 mol^-1
static constexpr double k_B = 1;          // J K^-1
static constexpr double T_static = 1;     // K

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
T log_with_base(T base, T x)
{
    return std::log(x)/std::log(base);
}

template<typename T>
struct sigmoid_wrapper
{
    T& value;

    sigmoid_wrapper(T& value_) : value{value_} {}

    operator T&() { return value; }
    operator T() const { return value; }

    template<typename Y>
    friend std::ostream& operator<< (std::ostream &out, const sigmoid_wrapper<Y>& sigmoid);

    T operator =(T value_) {return sigmoid(value_);}
    T operator =(T value_) const {return sigmoid(value_);}

    double
    sigmoid(double in)
    {
        return value = 1/(1+std::abs(in));
    }
};

template<typename T>
std::ostream& operator<< (std::ostream &out, const sigmoid_wrapper<T>& sigmoid)
{
    out << sigmoid.value;
 
    return out;
}

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

template<typename T>
struct Summation
{
    using sum_func_t = std::function<T(const T&)>;

    Summation(T start_, T end_, T step_, sum_func_t func) : start{start_}, end{end_}, step{step_}, func_{func} {}

    T operator() ()
    {
        T sum {0};
        T p {start};

        // Should really implement a comparison suitable for floating point numbers
        for(; p <= end; p += step)
            sum += func_(p);

        return sum;
    }

    T start;
    T end;
    T step;
    sum_func_t func_;
};