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

struct System
{
    double T;           // Temperature
    double friction;    // Friction coefficient
    double rho;         // Density
};

using System_ptr = std::shared_ptr<System>;

class System_physics : public Physics
{
    public:
        System_physics(System_ptr system) : system_{system} {}

        virtual void apply(Context&) = 0;

    protected:
        System_ptr system_;
};

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

// Depends: N_e, b
struct Tau_e : public System_physics
{
    Tau_e(System_ptr system) : System_physics{system} {}

    void apply(Context& ctx) override
    {
        ctx.tau_e = square(ctx.N_e) * system_->friction * square(ctx.b) / (3.0 * square(pi) * k_B * system_->T);
    }
};

// Depends: N_e, b
struct Tau_e_alt : public System_physics
{
    Tau_e_alt(System_ptr system) : System_physics{system} {}

    void apply(Context& ctx) override
    {
        ctx.tau_e = ctx.tau_monomer * square(ctx.N_e) / 3.0 * square(pi);
    }
};

// Depends: N_e, b
struct Tau_monomer : public System_physics
{
    Tau_monomer(System_ptr system) : System_physics{system} {}

    void apply(Context& ctx) override
    {
        ctx.tau_monomer = system_->friction * square(ctx.b) / (k_B * system_->T);
    }
};

// Depends: M_e
struct G_e : public System_physics
{
    G_e(System_ptr system) : System_physics{system} {}

    void apply(Context& ctx) override
    {
        ctx.G_e = system_->rho*gas_constant*system_->T/ctx.M_e;
    }
};

// Depends: Z, tau_e
struct Tau_r : public Physics
{
    void apply(Context& ctx) override
    {
        ctx.tau_r = square(ctx.Z) * ctx.tau_e;
    }
};

// Depends: Z, tau_e
struct Tau_d_0 : public Physics
{
    void apply(Context& ctx) override
    {
        ctx.tau_d_0 = 3 * std::pow(ctx.Z, 3.0) * ctx.tau_e;
    }
};

// Depends: Z, tau_d_0
struct Tau_df : public Physics
{
    void apply(Context& ctx) override
    {
        const double pow_Z =  std::pow(ctx.Z, 3.0/2.0);
        const double sqrt_Z = std::sqrt(ctx.Z);

        ctx.tau_df = ctx.tau_d_0*(1.0 - 3.38/sqrt_Z + 4.17/ctx.Z - 1.55/pow_Z);
    }
};