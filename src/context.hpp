#pragma once

#include <boost/hana.hpp>
#include <boost/hana/define_struct.hpp>
#include <boost/hana/keys.hpp>
#include <boost/hana/tuple.hpp>

#define LIME_KEY(x) BOOST_HANA_STRING(#x)

#include "utilities.hpp"

#include <vector>
#include <memory>
#include <functional>

struct Physics
{
    virtual ~Physics() = default;
    virtual void apply(struct Context&) = 0;
};

struct Context
{   
    using Physics_ptr = std::unique_ptr<Physics>;

public: 
    double M; // Chain mass
    double N;                      // Chain length
    double N_e;                    // Number of monomers per segment
    double M_e;                    // Normalized chain mass
    double Z;                      // Number of tube segments
    double a;                      // Persistence length
    double b;                      // Kuhn segment length
    double G_e;                    // Elastic modulus
    double G_f_normed;             // Dimensionless plateau
    double tau_e;                  // Entanglement time
    double tau_d_0;
    double tau_r; // Rouse time
    double tau_df;
    double tau_monomer; // Monomer relaxation time

    // Use this to attach physics that describe or change internal state
    // Several builders are available that set sane defaults for different tasks
    // Physics objects are owned by the context
    void add_physics(Physics_ptr physics);

    template <typename Physics_t, typename... Args>
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

private:
    std::vector<Physics_ptr> physics_;
    std::vector<struct Compute*> computes_;

    void notify_computes();
};

struct Compute
{
    virtual ~Compute() = default;
    virtual void update(const Context& ctx) = 0;
    virtual void validate_update(const Context& ctx) const = 0;
};

BOOST_HANA_ADAPT_STRUCT(Context,
    M, N, N_e, M_e, Z, a, b, G_e, G_f_normed, tau_e, tau_d_0, tau_r, tau_df, tau_monomer);

class IContext_view
{
private:
    virtual std::ostream& serialize(std::ostream&, const char* prefix = "\0") const = 0;

public:
    explicit IContext_view(const Context& ctx);
    virtual ~IContext_view() = default;
    virtual void accept(std::function<void(double&)> f) const = 0;

    friend std::ostream& operator << (std::ostream& stream, const IContext_view& view);
    friend struct Writer& operator << (struct Writer& stream, const IContext_view& view);

    const Context& context_;
};

template<const auto& keys>
class Context_view : public IContext_view
{
    public:
        explicit Context_view(const Context& ctx)
        :   IContext_view{ctx}
        {}

        void
        accept(std::function<void(double&)> f) const override
        {
            namespace hana = boost::hana;
            using namespace hana::literals;

            auto get_values = [&](auto const& obj) {
                return [&](auto const& ...key) {
                    return hana::make_tuple(hana::at_key(obj, key)...);
                };
            };

            auto vals = hana::unpack(keys, get_values(context_));
            hana::for_each(vals, f);
        }

    private:
        std::ostream&
        serialize(std::ostream& stream, const char* prefix = "\0") const override
        {
            namespace hana = boost::hana;
            using namespace hana::literals;

            auto get_values = [&](auto const& obj) {
                return [&](auto const& ...key) {
                    ((stream << prefix << key.c_str() << " " << hana::at_key(obj, key) << '\n'), ...);
                };
            };

            hana::unpack(keys, get_values(context_));
 
            return stream;
        }
};

class IContext_builder
{
    public:
        IContext_builder();
        virtual ~IContext_builder();
        IContext_builder(std::shared_ptr<Context>);

        virtual void gather_physics() = 0;
        virtual void initialize() = 0;
        virtual void validate_state() = 0;
        virtual std::unique_ptr<IContext_view> context_view() = 0;

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
        std::unique_ptr<IContext_view> context_view() override;

    protected:
        std::shared_ptr<System> system_;
};

class Reproduction_context_builder : public IContext_builder
{
    public:
        Reproduction_context_builder();
        Reproduction_context_builder(std::shared_ptr<Context>);

        std::unique_ptr<IContext_view> context_view() override;
        void gather_physics() override;
        void initialize() override;
        void validate_state() override;
};
