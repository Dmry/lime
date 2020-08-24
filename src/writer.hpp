#pragma once

/*
 *  This file is part of Lime, a tool for the application of Likhtman & McLiesh' model for polymer dyanmics.
 *
 *  Copyright © 2020 Daniel Emmery (CNRS)
 *
 *  File contents:
 * 
 *  * Utility for writing Context & Time_series data types to file (Writer)
 *  * Utility for writing an arbitrary number of vectors to file (Vector_writer)
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
    std::filesystem::path get_path() const {return current_path_;}
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

template <typename T, typename = void>
struct is_write_compatible : std::false_type
{
};

template <typename T>
struct is_write_compatible<T, std::void_t<
                                  decltype(std::declval<T>().size()),
                                  decltype(std::declval<T>().operator[](std::declval<typename T::size_type>()))>> : std::true_type
{
};

template <template <class> typename... Policies>
struct Vector_writer : public Policies<Vector_writer<Policies...>>...
{
    std::filesystem::path path;

    Vector_writer(std::filesystem::path path) : path{path}
    {
    }

    template <typename... T>
    void
    write(const T &... out)
    {
        static_assert((is_write_compatible<T>() && ...), "Can only pass containers that have .size() and operator[].");
        (static_cast<Policies<Vector_writer<Policies...>> &>(*this).write(out...), ...);
    }
};

template <typename U, typename... T>
size_t
size_helper(const U &first, const T &...)
{
    return first.size();
}

template <typename Derived>
struct dat
{
    template <typename... T>
    void
    write(T &&... out)
    {
        auto base = static_cast<Derived &>(*this);

        std::ofstream out_file{base.path};

        assert((out.size() == ...));

        for (size_t i = 0; i < size_helper(out...); ++i)
        {
            ((out_file << out[i] << ' '), ...) << std::endl;
        }

        out_file.flush();
    }
};