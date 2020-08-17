#pragma once

/*
 *  This file is part of Lime, a tool for the application of Likhtman & McLiesh' model for polymer dyanmics.
 *
 *  Copyright © 2020 Daniel Emmery (CNRS)
 *
 *  File contents:
 * 
 *  Configuration of STL parallel algorithms
 *
 *  GPL 3.0 License
 * 
 */

#include <execution>

#ifdef RUN_PARALLEL
    inline constexpr auto exec_policy = std::execution::par_unseq;
#else
    inline constexpr auto exec_policy = std::execution::seq;
#endif
