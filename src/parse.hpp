#pragma once

/*
 *  This file is part of Lime, a tool for the application of Likhtman & McLiesh' model for polymer dyanmics.
 *
 *  Copyright © 2020 Daniel Emmery (CNRS)
 *
 *  File contents:
 * 
 *  Parses files with arbitrary delimiters into arbitrary data types.
 *  This cuts some corners for the sake of speed and assumes relatively small, well formed files, so do heed the warnings.
 * 
 *  GPL 3.0 License
 * 
 */

#include <string>
#include <string_view>
#include <vector>
#include <cctype>
#include <iostream>
#include <functional>
#include <fstream>
#include <filesystem>
#include <limits>
#include <any>

using OType = std::vector<std::vector<std::string_view>>;
using Splitfunc = std::function<void(OType&, std::string_view, std::string_view, std::string_view)>;

// Find if current character is in compile time list
template<char... char_list>
struct is_one_of {
    private:
        constexpr static
        bool compare(char)
        {
            return false;
        }

        template<class ...other_char_list>
        constexpr static
        bool
        compare(char c, char this_one, other_char_list...others)
        {
            return c == this_one or compare(c, others...);
        }

    public:
        static
        bool
        check(char c)
        {
            return compare(c, char_list...);
        }
};

// Provides overloading of lambdas through overload{lambda, lambda...}
template<class... Ts> struct overload : Ts... { using Ts::operator()...; };
template<class... Ts> overload(Ts...) -> overload<Ts...>;

auto split_sub = overload{
    [](std::string_view res, std::vector<std::string_view>& out)
    {
        out.emplace_back(res);
    },
    [](std::string_view res, OType& out, std::string_view delim, std::function<void(OType&, std::string_view, std::string_view)> split)
    {
        split(out, res, delim);
    }
};

// Split string by delimiter and use callback F to handle (store or process) result
template<typename F>
void
split_base(std::string_view input, std::string_view delims, F f)
{
    auto first = input.begin();

	while (first != input.end())
	{
		const auto second = std::find_first_of(first, input.cend(), delims.begin(), delims.end());
		if (first != second)
		{
            auto res = input.substr(std::distance(input.begin(), first), std::distance(first, second));
			f(res);
		}

		if (second == input.end())
			break;

		first = std::next(second);
	}
}

// Split by delim_nested and store in output
void
split(OType& output, std::string_view input, std::string_view delim_nested)
{
	std::vector<std::string_view> out;

    split_base(input, delim_nested, std::bind(split_sub, std::placeholders::_1, std::ref(out)));

	output.emplace_back(out);
}


// Split by delim_eol ('\n') and call back split for any nested delimiters
void
split_recurse(OType& output, std::string_view input, std::string_view delim_eol, std::string_view delim_nested)
{
    split_base(input, delim_eol, std::bind(split_sub, std::placeholders::_1, std::ref(output), delim_nested, split));
}

// Detect and remove comments at the start of the stream.
struct clear_comments
{
    static void
    sanitize(std::istream& stream)
    {
        for (char c = static_cast<char>(stream.peek()); is_one_of<'#'>::check(c); c = static_cast<char>(stream.peek()))
        {
            stream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
    }
};

// Detect, store and remove headers at the start of the stream.
struct store_headers
{
    static std::vector<std::string> headers;

    static void
    sanitize(std::istream& stream)
    {
        if (not std::isdigit(stream.peek()))
        {
            std::string headerline;

            std::getline(stream, headerline);

            std::istringstream ss;

            while(std::getline(ss, headerline, ','))
            {
                headers.emplace_back(ss.str());
            }
        }
    }   
};

// Static init for struct store_headers
std::vector<std::string> store_headers::headers = {};

// Sets up sequence of sanitizers at compile time
// Usage: Sanitize_helper<sanitizers...>()(stream);
template <class ...other_sanitizers> struct Sanitize_helper;

template <>
struct Sanitize_helper<>
{
    void operator()(std::istream&)
    {}
};

template <class... sanitizers>
struct Sanitize_helper
{
    constexpr void operator()(std::istream& stream)
    {
        (sanitizers::sanitize(stream), ...);
    }
};

/*
 *  Remove characters from start or end of the stream before reading
 *
 *  WARNING: Order absolutely matters! If you expect a file to start with
 *           a comment, don't pass the header sanitizer before the comment
 *           sanitizer.
 *          
 */ 
template <class ...sanitizers>
void sanitize(std::istream& stream)
{
    Sanitize_helper<sanitizers...>()(stream);
}

// Buffers file contents
struct Buffer
{
    std::string buffer = {};

    // Read entire stream into buffer, using policies implemented by sanitizers
    template <class... sanitizers>
    void
    read_stream(std::istream& stream)
    {
        sanitize<sanitizers...>(stream);

        auto first = stream.tellg();

        stream.seekg(first, std::ios::end);

        buffer.resize(stream.tellg());
        stream.seekg(first);
        stream.read(buffer.data(), buffer.size());
    }

    // Clear buffer contents
    void
    clear()
    {
        buffer.clear();
    }
};

constexpr int_fast8_t STREAM_EMPTY = -1;

template<class file_format_t, class... sanitizers>
void
parse(file_format_t& file_format, std::filesystem::path path)
{   
    // Somehow this compiles on GCC, but using plain file_format.buffer doesn't..
    Buffer& buffer = file_format.buffer;

    // If no path specified, contents might be passed as stdin
    if (path.empty())
    {
        if (std::cin.tellg() == STREAM_EMPTY) throw std::runtime_error("Please specify a file path.");
        else buffer.read_stream<sanitizers...>(std::cin);
    }
    else if (not std::filesystem::exists(path))
    {
        throw std::runtime_error("Path '" + path.string() + "' not found.");
    }
    else
    {
        std::ifstream stream(path);
        buffer.read_stream<sanitizers...>(stream);
    }

    // Split by EOL and delimiters respectively
    // Sotred in file_format.out
    file_format.tokenize(buffer.buffer, file_format.delims);
};

template<class file_format_t, class... sanitizers>
file_format_t
parse_file(std::filesystem::path path)
{
    file_format_t file_format = {};
    
    parse<file_format_t, sanitizers...>(file_format, std::move(path));

    return file_format;
}

// Convert parsed file contents in column idx and return as vector
template<typename T>
struct Get
{
    static std::vector<T> col(size_t idx, OType& in)
    {
        if (idx >= in.front().size())
            throw std::runtime_error("Selected column out of range!");

        std::vector<T> out;

        for (auto row_ : in)
        {
            if constexpr(std::is_same<float, typename std::remove_cv<T>::type>::value or std::is_same<double, typename std::remove_cv<T>::type>::value)
            {
                try
                {
                    out.emplace_back(std::atof(row_.at(idx).data()));
                }
                catch (...)
                {
                    // WARNING: ignores line if error
                }
            }
            else if constexpr(std::is_same<int, typename std::remove_cv<T>::type>::value)
            {
                out.emplace_back(std::atoi(row_[idx].data()));
            }
            else
            {
                T result;
                std::stringstream ss(row_[idx].data());
                ss >> result;
                out.emplace_back(result);
            } 
        }

        return out;
    }
};

// Curiously recurring template format to allow the user to configure file specs
template<class Derived>
struct file_format
{
    Buffer buffer;
    OType out;
    std::string_view eol;
    std::string_view delims;

    // Splits buffered file contents by EOL and by delimiters.
    // Results stored in file_format.out
    std::function<void(std::string_view, std::string_view)> tokenize =
        [this] (std::string_view mmap_file, std::string_view delimiters) { split_recurse(out, mmap_file, eol, delimiters); };
};