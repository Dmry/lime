#pragma once

/*
 *  This file is part of Lime, a tool for the application of Likhtman & McLiesh' model for polymer dyanmics.
 *
 *  Copyright © 2020 Daniel Emmery (CNRS)
 *
 *  File contents:
 * 
 *  CUDA implementation for Rubinstein & Colby's method for constraint release
 *  Doi: 10.1063/1.455620
 *
 *  GPL 3.0 License
 * 
 */

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