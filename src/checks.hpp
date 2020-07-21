#pragma once

#include <boost/hana.hpp>

#include "context.hpp"

#include <string>
#include <stdexcept>

namespace checks
{
    template<typename... check_types>
    void
    check(IContext_view* view)
    {
        (view->accept(check_types()), ...);
    }

    template<typename T, typename... check_types>
    void
    check(const T& checkable)
    {
        (boost::hana::for_each(checkable, check_types()), ...);
    }

    namespace policies
    {
        struct tag
        {};

        struct throws : public tag
        {
            static void apply(const std::string& msg)
            {
                throw std::runtime_error(msg);
            }
        };

        struct prints_error : public tag
        {
            static void apply(const std::string& msg)
            {
                BOOST_LOG_TRIVIAL(error) << msg;
            }
        };

        template<const std::string& append>
        struct prints_error_append : public tag
        {
            static void apply(const std::string& msg)
            {
                BOOST_LOG_TRIVIAL(error) << msg << " " << append;
            }
        };
    }

    template<class... policies_t>
    struct is_nan
    {
        template<typename T>
        void operator()(T& t) const
        {
            if (t != t)
            {
                (policies_t::apply("found NaN"), ...);
            }
        }
    };

    template<class... policies_t>
    struct zero
    {
        template<typename T>
        void operator()(T& t) const
        {
            if (std::abs(t - t) > std::numeric_limits<T>::epsilon())
            {
                (policies_t::apply("found zero value"), ...);
            }
        }
    };

    template<class... policies_t>
    struct negative
    {
        template<typename T>
        void operator()(T& t) const
        {
            if (t < 0.0)
            {
                (policies_t::apply("found negative value"), ...);
            }
        }
    };
}