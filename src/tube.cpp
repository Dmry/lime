/*
 *
 *  Copyright © 2020 Daniel Emmery (CNRS) 
 * 
 *  Datatypes for reptation tube representations.
 * 
 *  GPL 2.0 License
 * 
 */

#include "tube.hpp"

System_physics::System_physics(System_ptr system)
: system_{system}
{}

System_physics::~System_physics()
{}

void G_f_normed::apply(Context &ctx)
{
    double Gf = 1.0 - (1.69 / std::sqrt(ctx.Z)) + (2.0 / ctx.Z) - (1.24 / std::pow(ctx.Z, 3.0 / 2.0));
    ctx.G_f_normed = 8.0 * Gf / square(pi);
}

void Z_from_length::apply(Context &ctx)
{
    ctx.Z = ctx.N / ctx.N_e;
}

void Z_from_mass::apply(Context &ctx)
{
    ctx.Z = ctx.M / ctx.M_e;
}

void N_e_from_tube::apply(Context &ctx)
{
    ctx.N_e = square(ctx.a) / square(ctx.b);
}

void N_e_from_M_e::apply(Context &ctx)
{
    ctx.N_e = ctx.M_e;
}

void N_e_from_Z::apply(Context &ctx)
{
    ctx.N_e = ctx.N / ctx.Z;
}

void M_e_from_N_e::apply(Context &ctx)
{
    ctx.M_e = ctx.N_e;
}

void M_e_from_Z::apply(Context &ctx)
{
    ctx.M_e = ctx.M / ctx.Z;
}

void Tau_e::apply(Context &ctx)
{
    ctx.tau_e = square(ctx.N_e) * system_->friction * square(ctx.b) / (3.0 * square(pi) * k_B * system_->T);
}

void Tau_e_alt::apply(Context &ctx)
{
    ctx.tau_e = ctx.tau_monomer * square(ctx.N_e) / 3.0 * square(pi);
}

void Tau_monomer::apply(Context &ctx)
{
    ctx.tau_monomer = system_->friction * square(ctx.b) / (k_B * system_->T);
}

void G_e::apply(Context &ctx)
{
    ctx.G_e = system_->rho * gas_constant * system_->T / ctx.M_e;
}

void Tau_r::apply(Context &ctx)
{
    ctx.tau_r = square(ctx.Z) * ctx.tau_e;
}

void Tau_d_0::apply(Context &ctx)
{
    ctx.tau_d_0 = 3.0 * std::pow(ctx.Z, 3.0) * ctx.tau_e;
}

void Tau_df::apply(Context &ctx)
{
    const double pow_Z = std::pow(ctx.Z, 3.0 / 2.0);
    const double sqrt_Z = std::sqrt(ctx.Z);

    ctx.tau_df = ctx.tau_d_0 * (1.0 - 3.38 / sqrt_Z + 4.17 / ctx.Z - 1.55 / pow_Z);
}
