#pragma once

/*
 *
 *  Copyright © 2020 Daniel Emmery (CNRS) 
 * 
 *  Datatypes for reptation tube representations.
 * 
 *  GPL 2.0 License
 * 
 */

#include "utilities.hpp"
#include "context.hpp"

#include <cmath>

// Depends: Z
struct G_f_normed : public Physics
{
    void apply(Context& ctx) override
    {
        double Gf = 1.0 - (1.69/std::sqrt(ctx.Z)) + (2.0/ctx.Z) - (1.24/std::pow(ctx.Z, 3.0/2.0));
        ctx.G_f_normed = 8.0*Gf/square(pi);
    };
};

// Depends: N, N_e
struct Z_from_length : public Physics
{
    void apply(Context& ctx) override
    {
        ctx.Z = ctx.N / ctx.N_e;
    }
};

// Depends: M, M_e
struct Z_from_mass : public Physics
{
    void apply(Context& ctx) override
    {
        ctx.Z = ctx.M / ctx.M_e;
    }
};

// Depends: a, b
struct N_e_from_tube : public Physics
{
    void apply(Context& ctx) override
    {
        ctx.N_e = square(ctx.a) / square(ctx.b);
    }
};

// Depends: M_e
struct N_e_from_M_e : public Physics
{
    void apply(Context& ctx) override
    {
        ctx.N_e = ctx.M_e;
    }
};

// Depends: N, Z
struct N_e_from_Z : public Physics
{
    void apply(Context& ctx) override
    {
        ctx.N_e = ctx.N / ctx.Z;
    }
};

// Depends: N_e
struct M_e_from_N_e : public Physics
{
    void apply(Context& ctx) override
    {
        ctx.M_e = ctx.N_e;
    }
};

// Depends: M, Z
struct M_e_from_Z : public Physics
{
    void apply(Context& ctx) override
    {
        ctx.M_e = ctx.M / ctx.Z;
    }
};