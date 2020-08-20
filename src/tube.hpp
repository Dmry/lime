#pragma once

/*
 *  This file is part of Lime, a tool for the application of Likhtman & McLiesh' model for polymer dyanmics.
 *
 *  Copyright © 2020 Daniel Emmery (CNRS)
 *
 *  File contents:
 * 
 *  Datatypes for tube representations.
 * 
 *  GPL 3.0 License
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
        System_physics(System_ptr system);
        virtual ~System_physics();

        virtual void apply(Context&) = 0;

    protected:
        System_ptr system_;
};

// Depends: Z
struct G_f_normed : public Physics
{
    void apply(Context& ctx) override;
};

// Depends: N, N_e
struct Z_from_length : public Physics
{
    void apply(Context& ctx) override;
};

// Depends: M, M_e
struct Z_from_mass : public Physics
{
    void apply(Context& ctx) override;
};

// Depends: a, b
struct N_e_from_tube : public Physics
{
    void apply(Context& ctx) override;
};

// Depends: M_e
struct N_e_from_M_e : public Physics
{
    void apply(Context& ctx) override;
};

// Depends: N, Z
struct N_e_from_Z : public Physics
{
    void apply(Context& ctx) override;
};

// Depends: N_e
struct M_e_from_N_e : public Physics
{
    void apply(Context& ctx) override;
};

// Depends: M, Z
struct M_e_from_Z : public Physics
{
    void apply(Context& ctx) override;
};

// Depends: N_e, b
struct Tau_e : public System_physics
{
    Tau_e(System_ptr system) : System_physics{system} {}

    void apply(Context& ctx) override;
};

// Depends: N_e, b
struct Tau_e_alt : public System_physics
{
    Tau_e_alt(System_ptr system) : System_physics{system} {}

    void apply(Context& ctx) override;
};

// Depends: N_e, b
struct Tau_monomer : public System_physics
{
    Tau_monomer(System_ptr system) : System_physics{system} {}

    void apply(Context& ctx) override;
};

// Depends: M_e
struct G_e : public System_physics
{
    G_e(System_ptr system) : System_physics{system} {}

    void apply(Context& ctx) override;
};

// Depends: Z, tau_e
struct Tau_r : public Physics
{
    void apply(Context& ctx) override;
};

// Depends: Z, tau_e
struct Tau_d_0 : public Physics
{
    void apply(Context& ctx) override;
};

// Depends: Z, tau_d_0
struct Tau_df : public Physics
{
    void apply(Context& ctx) override;
};
