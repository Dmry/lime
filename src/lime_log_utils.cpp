#include "lime_log_utils.hpp"

Async_except* Async_except::inst = nullptr;

Async_except::Async_except()
{}

Async_except* Async_except::get()
{
    if (!inst)
    {
        inst = new Async_except;
    }

    return inst;
}

void lime_log::print_exception(const std::exception& e, int level)
{
    BOOST_LOG_TRIVIAL(fatal) << std::string(level, ' ') << "exception: " << e.what();
    try {
        std::rethrow_if_nested(e);
    } catch(const std::exception& e) {
        print_exception(e, level+1);
    } catch(...) {}
}

void lime_log::coloring_formatter(boost::log::record_view const& rec, boost::log::formatting_ostream& strm)
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
            strm << "\033[33m" << "[WARNING] ";
            break;
        case boost::log::trivial::error:
            strm << "\033[31m" << "[ERROR] ";
            break;
        case boost::log::trivial::fatal:
            strm << "\033[31m" << "[FATAL] ";
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