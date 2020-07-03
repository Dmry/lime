#pragma once

/*
 *
 *  Copyright © 2020 Daniel Emmery (CNRS)
 * 
 *  GPL 2.0 License
 * 
 */

#include <vector>
#include <memory>
#include <cmath>

#include "context.hpp"
#include "utilities.hpp"

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

        virtual void apply(Context&) override {};

    protected:
        System_ptr system_;
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
struct Tau_r : public System_physics
{
    Tau_r(System_ptr system) : System_physics{system} {}

    void apply(Context& ctx) override
    {
        ctx.tau_r = square(ctx.Z) * ctx.tau_e;
    }
};

// Depends: Z, tau_e
struct Tau_d_0 : public System_physics
{
    Tau_d_0(System_ptr system) : System_physics{system} {}

    void apply(Context& ctx) override
    {
        ctx.tau_d_0 = 3 * std::pow(ctx.Z, 3.0) * ctx.tau_e;
    }
};

// Depends: Z, tau_d_0
struct Tau_df : public System_physics
{
    Tau_df(System_ptr system) : System_physics{system} {}

    void apply(Context& ctx) override
    {
        const double pow_Z =  std::pow(ctx.Z, 3.0/2.0);
        const double sqrt_Z = std::sqrt(ctx.Z);

        ctx.tau_df = ctx.tau_d_0*(1.0 - 3.38/sqrt_Z + 4.17/ctx.Z - 1.55/pow_Z);
    }
};


