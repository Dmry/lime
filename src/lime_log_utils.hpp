#pragma once

#include <boost/log/trivial.hpp>
#include <boost/log/attributes/value_extraction.hpp>
#include <boost/log/attributes/attribute_value.hpp>
#include <boost/log/utility/formatting_ostream.hpp>
#include <boost/log/utility/setup/console.hpp>

#include <stdexcept>
#include <functional>

// NOT THREAD SAFE, meant for parallel std::algorithms only
struct Async_except
{
    private:
        static Async_except* inst;

    public:
        static Async_except* get();
        std::exception_ptr ep;

    protected:
        Async_except();
};

template<typename T, typename F>
void
info_or_warn(const std::string& description, const T& var, F pred)
{
    if (pred(var))
    {
        BOOST_LOG_TRIVIAL(warning) << description << ": " << var;
    }
    else
    {
        BOOST_LOG_TRIVIAL(info) << description << ": " << var;
    }
}

namespace lime_log
{
    void print_exception(const std::exception& e, int level =  0);  
    void coloring_formatter(boost::log::record_view const& rec, boost::log::formatting_ostream& strm);
}
