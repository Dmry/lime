#include <boost/fusion/include/for_each.hpp>
#include <boost/fusion/include/algorithm.hpp>
#include <boost/phoenix/phoenix.hpp>
#include <boost/mpl/range_c.hpp>

#include "context.hpp"
#include "utilities.hpp"

#include <iostream>
#include <stdexcept>

void Context::add_physics(Physics_ptr physics)
{
    physics_.emplace_back(std::move(physics));
}

void Context::apply_physics()
{
    for (auto& calculation : physics_)
        calculation->apply(*this);

    notify_computes();
}

void Context::attach_compute(Compute* compute)
{
    computes_.emplace_back(compute);
}

void Context::attach_compute(std::vector<Compute*> computes)
{
    using std::begin, std::end;
    computes_.insert(end(computes_), begin(computes), end(computes));
}

void Context::notify_computes() 
{
    for (auto compute : computes_)
    {
        if (compute != nullptr)
        {
            compute->update(*this);
        }
    }
}

struct check_nan_impl
{
    template<typename T>
    void operator()(T& t) const
    {
        if (t != t)
        {
            throw std::runtime_error("Got NaN");
        }
    }
};

// Checks if any of the values in the context is NaN
void Context::check_nan()
{
    using boost::phoenix::arg_names::arg1;

    boost::fusion::for_each(*this, check_nan_impl());
}

// Prints name of every variable in the context and its current value
// Logs to info if > 0, logs to warning if < 0
void Context::print()
{
    // Credit: Joao Tavora
    boost::fusion::for_each(boost::mpl::range_c<
        unsigned, 0, boost::fusion::result_of::size<Context>::value>(),
            [&](auto index){
                info_or_warn(boost::fusion::extension::struct_member_name<Context,index>::call(), boost::fusion::at_c<index>(*this));
            }
    );
}