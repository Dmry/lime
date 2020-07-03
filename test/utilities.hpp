#pragma once

#include <catch2/catch.hpp>

#include "../src/utilities.hpp"

Time_range::type test_time_range();
Time_range::type test_exponential_time_range();

#define test_entanglement_time() GENERATE(1.0, 200.0, 400.0, 700.0, 1e4)
#define test_segment_count() GENERATE(2.0, 50.0, 100.0, 200.0, 1000.0)
#define test_df() GENERATE(2.0, 50.0, 100.0, 200.0, 1000.0)
