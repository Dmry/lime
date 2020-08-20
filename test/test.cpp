#define BOOST_TEST_MAIN
#if !defined( WIN32 )
    #define BOOST_TEST_DYN_LINK
#endif

#define BOOST_TEST_MODULE test_constraint_release
#include <boost/test/unit_test.hpp>
#include <boost/test/included/unit_test.hpp>

/// Constraint release

#include "../src/constraint_release/constraint_release.hpp"
#include "../src/constraint_release/heuzey.hpp"
#include "../src/constraint_release/rubinsteincolby.hpp"
#include "../src/time_series.hpp"
#include "../src/file_reader.hpp"
#include "../src/postprocess.hpp"

/// Context

#include <boost/test/tools/output_test_stream.hpp>

#include "../src/context.hpp"
#include "../src/checks.hpp"
#include "../src/tube.hpp"

#include <array>
#include <numeric>
#include <filesystem>
#include <algorithm>
#include <memory>

static Register_class<IConstraint_release, RUB_constraint_release, constraint_release::impl, double, Context&> rubinstein_constraint_release_factory(constraint_release::impl::RUBINSTEINCOLBY);
static Register_class<IConstraint_release, HEU_constraint_release, constraint_release::impl, double, Context&> heuzey_constraint_release_factory(constraint_release::impl::HEUZEY);

struct Reproduction_context {
	using path_impl_pair = std::pair<std::filesystem::path, constraint_release::impl>;

	Reproduction_context()
	: ctx{std::make_shared<Context>()}
	{
		ctx->Z = 300;
		ctx->tau_e = 1;

		Reproduction_context_builder builder(ctx);
    	builder.gather_physics();
        builder.initialize();

    	ctx = builder.get_context();
    	ctx->apply_physics();
	}

	~Reproduction_context()
	{}

	void check_impl(std::filesystem::path path, constraint_release::impl impl)
	{
		Time_series input = File_reader::get_file_contents(path, 1);

        auto CR = constraint_release::Factory_observed::create(impl, 1.0, *ctx);

        auto wrapper = [&CR](const Time_series::value_primitive& t) {return (*CR)(t);};

        Time_series series = dimensionless_derivative(wrapper, input.get_time_range(), ctx);

        auto vals = series.get_values();
        auto inputs = input.get_values();

        for (size_t i{0} ; i < series.size() ; ++i)
        {
            BOOST_TEST_INFO("index: " << i);
            BOOST_CHECK_CLOSE(vals[i], inputs[i], 8.0);
        }
	}

	std::shared_ptr<Context> ctx;
};

BOOST_AUTO_TEST_CASE(
    constraint_release_factory,
    * boost::unit_test::label("factory")
    * boost::unit_test::label("heuzey")
    * boost::unit_test::label("rubinstein")
    * boost::unit_test::description("Test if constraint release factory returns an instance for given entried in constraint_release::impl."))
{
    std::array<constraint_release::impl, 2> impls{constraint_release::impl::HEUZEY, constraint_release::impl::RUBINSTEINCOLBY};

    Context ctx;

    for (auto impl : impls)
    {
        BOOST_TEST_INFO(impl);
        auto CR = constraint_release::Factory_observed::create(impl, 0.1, ctx);
        BOOST_CHECK( CR );
    }
}

BOOST_FIXTURE_TEST_CASE(
    lm_figure_six,
    Reproduction_context,
    * boost::unit_test::label("validation")
    * boost::unit_test::label("rubinstein")
    * boost::unit_test::description("Test Rubinstein & Colby method for constraint release against literature data. (doi:10.1063/1.455620)"))
{
    BOOST_TEST_INFO("This method uses stochastic sampling and may fail from time to time, while still providing relatively accurate results in production.");
    check_impl("../validation/likhtmanmcleish/figure6_R.dat", constraint_release::impl::RUBINSTEINCOLBY);
}

BOOST_FIXTURE_TEST_CASE(
    heuzey_figure_five,
    Reproduction_context,
    * boost::unit_test::label("validation")
    * boost::unit_test::label("heuzey")
    * boost::unit_test::description("Test Heuzey method for constraint release against literature data. (doi:10.1002/app.20881)"))
{
	check_impl("../validation/heuzey/figure5_R.dat", constraint_release::impl::HEUZEY);
}

struct Test_physics : public Physics
{
    void apply(Context& ctx) override
    {
        ctx.Z = ctx.G_e / ctx.N;
    }
};

struct Test_physics_self_divide : public Physics
{
    void apply(Context& ctx) override
    {
        ctx.Z = ctx.Z / ctx.N;
    }
};

struct Test_context
{
    Test_context()
    : ctx{Context()}
    {
        ctx.M = 1.0; ctx.N = 2.0; ctx.N_e = 3.0; ctx.M_e = 4.0; ctx.Z = 5.0; ctx.a = 6.0;
        ctx.b = 7.0; ctx.G_e = 8.0; ctx.G_f_normed = 9.0; ctx.tau_e = 10.0; ctx.tau_d_0 = 11.0;
        ctx.tau_r = 12.0; ctx.tau_df = 13.0; ctx.tau_monomer = 14.0;
    }

    Context ctx;
};

BOOST_FIXTURE_TEST_CASE(
    context_add_physics,
    Test_context,
    * boost::unit_test::label("context"))
{
    std::unique_ptr<Physics> physics_two = std::make_unique<Test_physics>();
    ctx.add_physics(std::move(physics_two));
    ctx.apply_physics();

    BOOST_TEST(ctx.Z == 4.0);

    std::unique_ptr<Physics> physics_three = std::make_unique<Test_physics_self_divide>();
    ctx.add_physics(std::move(physics_three));
    ctx.apply_physics();
    BOOST_TEST(ctx.Z == 2.0);
}

BOOST_FIXTURE_TEST_CASE(
    context_nullptr_physics,
    Test_context,
    * boost::unit_test::label("context"))
{
    std::unique_ptr<Physics> physics = nullptr;
    ctx.add_physics(std::move(physics));
    BOOST_REQUIRE_NO_THROW(ctx.apply_physics());
}

BOOST_FIXTURE_TEST_CASE(
    context_add_physics_in_place,
    Test_context,
    * boost::unit_test::label("context"))
{
    ctx.add_physics_in_place<Test_physics>();
    ctx.apply_physics();

    BOOST_TEST(ctx.Z == 4.0);
}

struct Test_compute : public Compute
{
    bool success_;
    bool validated_;

    Test_compute() : success_{false}, validated_{false} {}

    virtual void update(const Context& ctx) override
    {
        success_ = true;
        validate_update(ctx);
    };

    virtual void validate_update(const Context&) const override
    {
        if (success_ != true)
            throw std::runtime_error("validate_update failed.");
    };
};

BOOST_FIXTURE_TEST_CASE(
    context_computes_empty_physics,
    Test_context,
    * boost::unit_test::label("context"))
{
    Test_compute compute;

    ctx.attach_compute(&compute);

    ctx.apply_physics();

    BOOST_TEST(compute.success_ == true);
}


BOOST_FIXTURE_TEST_CASE(
    context_nullptr_compute_empty_physics,
    Test_context,
    * boost::unit_test::label("context"))
{
    Test_compute compute;

    ctx.attach_compute(nullptr);

    BOOST_REQUIRE_NO_THROW(ctx.apply_physics());
}

BOOST_FIXTURE_TEST_CASE(
    context_computes_after_physics,
    Test_context,
    * boost::unit_test::label("context"))
{
    ctx.add_physics_in_place<Test_physics>();

    Test_compute compute;

    ctx.attach_compute(&compute);

    ctx.apply_physics();

    BOOST_TEST(compute.success_ == true);
}

constexpr auto test_keys = boost::hana::make_tuple(
    LIME_KEY(Z),
    LIME_KEY(M_e),
    LIME_KEY(N),
    LIME_KEY(N_e),
    LIME_KEY(G_f_normed),
    LIME_KEY(tau_e),
    LIME_KEY(tau_monomer),
    LIME_KEY(G_e),
    LIME_KEY(tau_r),
    LIME_KEY(tau_d_0),
    LIME_KEY(tau_df)
);

BOOST_FIXTURE_TEST_CASE(
    context_view_stream_filter,
    Test_context,
    * boost::unit_test::label("context"))
{
    using boost::test_tools::output_test_stream;

    output_test_stream out;
    Context_view<test_keys> view(ctx);
    out << view;

    BOOST_TEST( !out.is_empty( false ) );
    BOOST_TEST( out.is_equal("Z 5\nM_e 4\nN 2\nN_e 3\nG_f_normed 9\ntau_e 10\ntau_monomer 14\nG_e 8\ntau_r 12\ntau_d_0 11\ntau_df 13\n"));
}

BOOST_FIXTURE_TEST_CASE(
    context_check_view_sane_passes,
    Test_context,
    * boost::unit_test::label("context")
    * boost::unit_test::label("checks"))
{
    using namespace checks;
    using namespace checks::policies;
    Context_view<test_keys> view(ctx);
    BOOST_REQUIRE_NO_THROW(check<zero<throws>>(&view));
    BOOST_REQUIRE_NO_THROW(check<is_nan<throws>>(&view));
}

BOOST_FIXTURE_TEST_CASE(
    context_check_view_only_subset_checked,
    Test_context,
    * boost::unit_test::label("context")
    * boost::unit_test::label("checks"))
{
    using namespace checks;
    using namespace checks::policies;
    Context_view<test_keys> view(ctx);

    ctx.a = 0; // not part of the test keys, so shouldn't throw
    ctx.b = std::numeric_limits<double>::quiet_NaN();

    BOOST_REQUIRE_NO_THROW(check<zero<throws>>(&view));
    BOOST_REQUIRE_NO_THROW(check<is_nan<throws>>(&view));
}

BOOST_FIXTURE_TEST_CASE(
    context_full_reproduction_builder_valid_state,
    Test_context,
    * boost::unit_test::label("context")
    * boost::unit_test::label("checks")
    * boost::unit_test::label("builders"))
{
    // Obiously not suitable production time use of a shared_ptr, _please_ do not use this as an example.
    // This is only useful because it mirrors the context that's used in all tests to a shared pointer for
    // the sake of testing the constructor, it makes the object otherwise non-functional.
    auto ctx_ptr = std::shared_ptr<Context>(&ctx, [](Context *) {});

    Reproduction_context_builder builder(ctx_ptr);

    // The following _is_ the preferred way of calling.
    builder.gather_physics();
    builder.initialize();
    BOOST_REQUIRE_NO_THROW( builder.validate_state() );
}

BOOST_AUTO_TEST_CASE(
    context_minimal_reproduction_builder_valid_state,
    * boost::unit_test::label("context")
    * boost::unit_test::label("checks")
    * boost::unit_test::label("builders"))
{
    auto minimal_ctx = std::make_shared<Context>();

    minimal_ctx->Z = 1.0;
    minimal_ctx->tau_e = 2.0;

    Reproduction_context_builder builder(minimal_ctx);

    builder.gather_physics();
    builder.initialize();
    BOOST_REQUIRE_NO_THROW( builder.validate_state() );
}

BOOST_AUTO_TEST_CASE(
    context_minimal_ICS_builder_valid_state,
    * boost::unit_test::label("context")
    * boost::unit_test::label("checks")
    * boost::unit_test::label("builders"))
{
    auto minimal_ctx = std::make_shared<Context>();
    auto minimal_sys = std::make_shared<System>();

    minimal_sys->T = 1.0;
    minimal_sys->rho = 0.68;

    minimal_ctx->N = 1.0;
    minimal_ctx->N_e = 2.0;
    minimal_ctx->tau_monomer = 3.0;

    ICS_context_builder builder(minimal_sys, minimal_ctx);

    builder.gather_physics();
    builder.initialize();
    BOOST_REQUIRE_NO_THROW( builder.validate_state() );
}

BOOST_FIXTURE_TEST_CASE(
    context_check_view_throws,
    Test_context,
    * boost::unit_test::label("context")
    * boost::unit_test::label("checks"))
{
    using namespace checks;
    using namespace checks::policies;
    Context_view<test_keys> view(ctx);

    ctx.Z = 0; // not part of the test keys, so shouldn't throw
    ctx.N = std::numeric_limits<double>::quiet_NaN();

    BOOST_REQUIRE_THROW( (check<zero<throws>>(&view)), std::runtime_error);
    BOOST_REQUIRE_THROW( (check<is_nan<throws>>(&view)), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(
    generic_check_sane_passes,
    * boost::unit_test::label("checks"))
{
    using namespace checks;
    using namespace checks::policies;

    auto tuple = boost::hana::make_tuple(1.0, 2.0, 3.0, 4.0);

    using tuple_t = decltype(tuple);

    BOOST_REQUIRE_NO_THROW( (check<tuple_t, zero<throws>, is_nan<throws>>(tuple)) );
}

BOOST_AUTO_TEST_CASE(
    generic_check_throws,
    * boost::unit_test::label("checks"))
{
    using namespace checks;
    using namespace checks::policies;

    auto tuple = boost::hana::make_tuple(0.0, std::numeric_limits<double>::quiet_NaN());

    using tuple_t = decltype(tuple);

    BOOST_REQUIRE_THROW( (check<tuple_t, zero<throws>>(tuple)), std::runtime_error );
    BOOST_REQUIRE_THROW( (check<tuple_t, is_nan<throws>>(tuple)), std::runtime_error );
}