#pragma once

#include <boost/log/trivial.hpp>
#include <boost/log/attributes/value_extraction.hpp>
#include <boost/log/attributes/attribute_value.hpp>
#include <boost/log/utility/formatting_ostream.hpp>
#include <boost/log/utility/setup/console.hpp>

#include <stdexcept>

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

struct lime_log
{
    static void coloring_formatter(boost::log::record_view const& rec, boost::log::formatting_ostream& strm)
    {
        auto severity = rec.attribute_values()["Severity"].extract<boost::log::trivial::severity_level>();
        if (severity)
        {
            // Set the color
            switch (severity.get())
            {
            case boost::log::trivial::info:
                strm << "\033[32m";
                break;
            case boost::log::trivial::warning:
                strm << "\033[33m";
                break;
            case boost::log::trivial::error:
            case boost::log::trivial::fatal:
                strm << "\033[31m";
                break;
            default:
                break;
            }
        }

        // Format the message here...
        strm << rec[boost::log::expressions::smessage];

        if (severity)
        {
            // Restore the default color
            strm << "\033[0m";
        }
    }
};