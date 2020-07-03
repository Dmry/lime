#pragma once

/*
 *
 *  Copyright © 2020 Daniel Emmery (CNRS) 
 * 
 *  Writes arbitrary number of vectors to file.
 *  Requires vector type to have .size() and operator
 * 
 *  GPL 2.0 License
 * 
 */


#include <filesystem>
#include <fstream>
#include <vector>
#include <cassert>
#include <type_traits>

template <typename T, typename = void>
struct is_write_compatible : std::false_type {};

template <typename T>
struct is_write_compatible<T, std::void_t<
    decltype(std::declval<T>().size()),
    decltype(std::declval<T>().operator[](std::declval<typename T::size_type>()))
    >> : std::true_type {};

template<template<class> typename... Policies>
struct Vector_writer : public Policies<Vector_writer<Policies...>>...
{
    std::filesystem::path path;

    Vector_writer() : path{"out.dat"}
    {}

    template<typename... T>
    void
    write(const T&... out)
    {
        static_assert((is_write_compatible<T>() && ...), "Can only pass containers that have .size() and operator[].");
        (static_cast<Policies<Vector_writer<Policies...>>&>(*this).write(out...), ...);
    }
};

template<typename U, typename... T>
size_t
size_helper(const U& first, const T&...)
{
    return first.size();
}

template<typename Derived>
struct dat
{
    template<typename... T>
    void
    write(T&&... out)
    {
        auto base = static_cast<Derived&>(*this);

        std::ofstream out_file{base.path};

        assert( (out.size() == ...) );

        for(size_t i = 0 ; i < size_helper(out...) ; ++i)
        {
            ((out_file << out[i] << ' ') , ...) << std::endl;
        }

        out_file.flush();
    }
};