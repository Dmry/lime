#pragma once

#include <boost/fusion/include/for_each.hpp>
#include <boost/fusion/include/algorithm.hpp>
#include <boost/phoenix/phoenix.hpp>
#include <boost/mpl/range_c.hpp>
#include <boost/tuple/tuple_io.hpp>

#include "context.hpp"

#include <fstream>
#include <filesystem>

struct Writer
{
    Writer(std::filesystem::path outpath) : current_path_{outpath}
    {}

    std::ofstream& get_stream() {if (not stream_.is_open()) stream_.open(current_path_); return stream_;}
    std::filesystem::path get_path() {return current_path_;}
    void set_path(std::filesystem::path path) {stream_.close(); current_path_ = path;}
    void set_filename(const std::string& new_filename) {stream_.close(); current_path_.replace_filename(new_filename);}

private:
    std::ofstream stream_;
    std::filesystem::path current_path_;
};

Writer& operator<< (Writer& writer, const Context& context)
{
    using namespace boost::fusion;

    for_each(boost::mpl::range_c <
        unsigned, 0, result_of::size<Context>::value>(),
            [&](auto index) constexpr {
                writer.get_stream() << "# " << extension::struct_member_name<Context,index>::call() << " " << at_c<index>(context) << "\n";
            }
    );
 
    return writer;
}

namespace boost{
namespace tuples{

template<typename T, typename U>
std::ostream & operator<<(Writer& writer, const boost::tuples::tuple<T, U>& tup)
{
    return writer << boost::get<0>(tup) << " " << boost::get<1>(tup);
};

}
}

template<typename T>
Writer& operator<< (Writer& writer, const T& val)
{
    writer.get_stream() << val;
    return writer;
}