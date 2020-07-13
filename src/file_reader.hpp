#include "parse.hpp"
#include "time_series.hpp"

struct ics_file_format : file_format<ics_file_format>
{
    ics_file_format()
    {
        eol = "\r\n";
        delims = " ,";
    }
};

struct File_reader
{
    static Time_series get_file_contents(std::filesystem::path inpath, size_t value_col, size_t time_col = 0)
    {
        Time_series::value_type values;
        Time_series::time_type time;

        auto file_contents = parse_file<ics_file_format, clear_comments, store_headers>(inpath);

        values = Get<Time_series::value_primitive>::col(value_col, file_contents.out);
        time = Time_range::convert(Get<double>::col(time_col, file_contents.out));

        file_contents.buffer.clear();

        return Time_series(time, values);
    }
};