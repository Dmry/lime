#pragma once

#include <vector>

struct Cu_me_details
{
    Cu_me_details();
    ~Cu_me_details();
    Cu_me_details(const Cu_me_details&);

    size_t size_;
    float *km_ptr_;
    unsigned int *res_;
    unsigned int *h_res_;
    void set_km(const std::vector<double>& vec);
    double cu_me(double epsilon, size_t realization_size, size_t realizations) const;
};