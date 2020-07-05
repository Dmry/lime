#pragma once

#include <boost/log/trivial.hpp>
#include <boost/log/attributes/value_extraction.hpp>
#include <boost/log/attributes/attribute_value.hpp>
#include <boost/log/utility/formatting_ostream.hpp>
#include <boost/log/utility/setup/console.hpp>

#define BOOST_LOG_DYN_LINK 1

struct ics_log
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