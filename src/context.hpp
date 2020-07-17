#pragma once

#include <boost/fusion/adapted/struct.hpp>

#include "utilities.hpp"

#include <vector>
#include <memory>
#include <functional>

struct Physics
{
    virtual void apply(struct Context&) = 0;
};

struct Context
{   
    using Physics_ptr = std::unique_ptr<Physics>;

    public:
        double M;           // Chain mass
        double N;           // Chain length
        double N_e;         // Number of monomers per segment
        double M_e;         // Normalized chain mass
        double Z;           // Number of tube segments
        double a;           // Persistence length
        double b;           // Kuhn segment length
        double G_e;         // Elastic modulus
        double G_f_normed;  // Dimensionless plateau 
        double tau_e;       // Entanglement time
        double tau_d_0;
        double tau_r;       // Rouse time
        double tau_df;
        double tau_monomer; // Monomer relaxation time

        // Use this to attach physics that describe or change internal state
        // Several builders are available that set sane defaults for different tasks
        // Physics objects are owned by the context
        void add_physics(Physics_ptr physics);

        template<typename Physics_t, typename... Args>
        void add_physics_in_place(Args... args)
        {
            add_physics(std::make_unique<Physics_t>(args...));
        }

        void apply_physics();

        // Computes are called whenever the context is updated according to the chosen 'internal' physics
        // Use this to attach external factors that depend on the context and need to be updated immediately upon state change
        // Compute objects are NOT owned by the context
        void attach_compute(struct Compute* compute);
        void attach_compute(std::vector<struct Compute*> computes);

        void print();

    private:
        std::vector<Physics_ptr> physics_;
        std::vector<struct Compute*> computes_;

        void notify_computes();
};

BOOST_FUSION_ADAPT_STRUCT(Context, M, N, N_e, M_e, Z, a, b, G_e, G_f_normed, tau_e, tau_d_0, tau_r, tau_df, tau_monomer);

struct Compute {
    virtual void update(const Context& ctx) = 0;
};

class IContext_builder
{
    public:
        IContext_builder();
        IContext_builder(std::shared_ptr<Context>);

        virtual void gather_physics() = 0;
        virtual void initialize() = 0;
        virtual void validate_state() = 0;

        void set_context(std::shared_ptr<Context>);
        std::shared_ptr<Context> get_context();

    protected:
        std::shared_ptr<Context> context_;
};

class ICS_context_builder : public IContext_builder
{
    public:
        ICS_context_builder(std::shared_ptr<struct System> sys);
        ICS_context_builder(std::shared_ptr<struct System> sys, std::shared_ptr<Context>);

        void gather_physics() override;
        void initialize() override;
        void validate_state() override;
    
    protected:
        std::shared_ptr<System> system_;
};

class Reproduction_context_builder : public IContext_builder
{
    public:
        Reproduction_context_builder();
        Reproduction_context_builder(std::shared_ptr<Context>);

        void gather_physics() override;
        void initialize() override;
        void validate_state() override;
};