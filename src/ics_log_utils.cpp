#include "ics_log_utils.hpp"

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