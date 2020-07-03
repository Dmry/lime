#pragma once

#ifdef TEST

#include <iostream>

#define ICS_LOG(LVL, param) //std::cerr << param << std::endl;
#define ICS_LOG_SPLIT_NEWLINES(LVL,param) //std::cerr << param << std::endl;

#else

#include <string>
#include <sstream>

#include "../inc/spdlog/spdlog.h"
#include "../inc/spdlog/sinks/stdout_color_sinks.h"

#define ICS_LOG(LVL, param) spdlog::get("console")->LVL(param)
#define ICS_LOG_SPLIT_NEWLINES(LVL,param) log_omit_newline(param, [](std::string_view out) {ICS_LOG(LVL, out);})

/// @brief Emits a new log message for each newline in a string.
template <typename F>
void log_omit_newline(std::string&& message, F outfunc)
{
    while (message.back() == '\n') message.erase(message.size() - 1);
    std::stringstream out(message); 
      
    std::string intermediate; 
      
    while(getline(out, intermediate, '\n')) 
    { 
        outfunc(intermediate);
    }
}
#endif