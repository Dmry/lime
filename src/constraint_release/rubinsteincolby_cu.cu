#include <vector>
#include <memory>
#include <iostream>
#include <stdexcept>

#include "rubinsteincolby_cu.hpp"

__global__ void
me(float epsilon, int realization_size, float* km, unsigned int* res, unsigned int max)
{
    unsigned int blk = blockIdx.x;
    unsigned int idx = threadIdx.x;
    const size_t stride = (blk*blockDim.x + idx)* realization_size;

    __shared__ unsigned int sres[1];

    if (idx < blockDim.x)
        sres[blk] = 0;
    if (idx == blockDim.x+1)
        res[0] = 0;

    __syncthreads();

    if (stride+realization_size < max)
    {
        float s{0.0};

        s = km[0+stride] + km[1+stride] - epsilon;
        if (s < 0.0) atomicAdd(&sres[blk], 1);

        for (size_t i = 1+stride; i < stride+realization_size-1 ; ++i)
        {
            s = km[i] + km[i+1] - epsilon - (km[i]*km[i])/s;
            if (s < 0.0) atomicAdd(&sres[blk], 1);
        }
    }
    __syncthreads();

    if (stride < blockDim.x)
        atomicAdd(&res[0], sres[blk]);

}

Cu_me_details::Cu_me_details()
: size_{0}, km_ptr_{nullptr}
{
    cudaMalloc((void**)&res_, sizeof(unsigned int));
    cudaMallocHost((void**)&h_res_, sizeof(unsigned int));
}

Cu_me_details::~Cu_me_details()
{
    cudaFree(km_ptr_);
    cudaFree(res_);
    cudaFreeHost(h_res_);
}

Cu_me_details::Cu_me_details(const Cu_me_details& other)
: size_{other.size_}
{
    cudaMalloc((void**)&km_ptr_, static_cast<int>(size_));
    if ( cudaSuccess != cudaGetLastError() )
        std::cout << "asdf" << std::endl;
    cudaMemcpy(km_ptr_, other.km_ptr_, size_, cudaMemcpyDeviceToDevice);
}

void
Cu_me_details::set_km(const std::vector<double>& vec)
{
    const std::vector<float> km_vec(vec.begin(), vec.end());

    cudaFree(km_ptr_);

    size_ = km_vec.size()*sizeof(float);

    cudaMalloc((void**)&km_ptr_, size_);

    cudaMemcpy(km_ptr_, km_vec.data(), size_, cudaMemcpyHostToDevice);
}

double
Cu_me_details::cu_me(double epsilon, size_t realization_size, size_t realizations) const
{
    me<<<1, realizations>>>(static_cast<float>(epsilon), static_cast<int>(realization_size), km_ptr_, res_, static_cast<unsigned int>(realization_size*realizations));

    cudaMemcpy(h_res_, res_, sizeof(unsigned int), cudaMemcpyDeviceToHost);

    return static_cast<double>(*h_res_)/static_cast<double>(realization_size*realizations);
}