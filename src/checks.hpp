#pragma once

/*
 *  This file is part of Lime, a tool for the application of Likhtman & McLiesh' model for polymer dyanmics.
 *
 *  Copyright © 2020 Daniel Emmery (CNRS)
 *
 *  File contents:
 * 
 *  Validation of context data types
 *
 *  GPL 3.0 License
 * 
 */

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
            static void apply [[noreturn]] (const std::string &msg)
            {
                throw std::runtime_error(msg);
            }
        };

        struct prints_warning : public tag
        {
            static void apply(const std::string& msg)
            {
                BOOST_LOG_TRIVIAL(warning) << msg;
            }
        };

        template<const std::string& append>
        struct prints_warning_append : public tag
        {
            static void apply(const std::string& msg)
            {
                BOOST_LOG_TRIVIAL(warning) << msg << " " << append;
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
            if (std::abs(t) < std::numeric_limits<T>::epsilon())
            {
                (policies_t::apply("a parameter is set to zero"), ...);
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
                (policies_t::apply("a parameter is set to a negative value"), ...);
            }
        }
    };
}
