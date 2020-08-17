#pragma once

/*
 *  This file is part of Lime, a tool for the application of Likhtman & McLiesh' model for polymer dyanmics.
 *
 *  Copyright © 2020 Daniel Emmery (CNRS)
 *
 *  File contents:
 * 
 *  Utility for writing Context & Time_series data types to file
 * 
 *  GPL 3.0 License
 * 
 */

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

Writer& operator<< (Writer& writer, const IContext_view& view)
{
    view.serialize(writer.get_stream(), "# ");
    return writer;
}

template<typename T>
Writer& operator<< (Writer& writer, const T& val)
{
    writer.get_stream() << val;
    return writer;
}