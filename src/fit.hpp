#pragma once

#include <tuple>
#include <functional>
#include <sstream>
#include <any>

#include <gsl/gsl_matrix.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_multifit_nlinear.h>

#include "lime_log_utils.hpp"
#include "time_series.hpp"
#include "result.hpp"

template <typename... T>
struct Fit
{
	struct User_data
  	{
		const Time_series::value_type* fitting_data_points;
		std::tuple<T&...>* free_variables; 
		ICS_result* result;
	};

	using Target_func = int(*)(const gsl_vector*, void*, gsl_vector*);

	gsl_multifit_nlinear_workspace *workspace;
	gsl_multifit_nlinear_fdf function;
	gsl_multifit_nlinear_parameters fdf_params;
	const gsl_multifit_nlinear_type *algorithm;
	std::tuple<T&...> free_variables;
	void (*callback_func)(const size_t, void*, const gsl_multifit_nlinear_workspace *w);
	void* callback_params;

	Fit(T&... free_variables_)
	: workspace{nullptr},
	  function{},
	  fdf_params{gsl_multifit_nlinear_default_parameters()},
	  algorithm{gsl_multifit_nlinear_trust},
	  free_variables{free_variables_...},
	  callback_func{&default_callback},
	  callback_params{nullptr}
	{
		fdf_params.scale = gsl_multifit_nlinear_scale_more;
		fdf_params.trs =   gsl_multifit_nlinear_trs_subspace2D;
		fdf_params.fdtype = GSL_MULTIFIT_NLINEAR_FWDIFF;
		fdf_params.solver = gsl_multifit_nlinear_solver_svd;
		fdf_params.factor_up = 3;
		fdf_params.factor_down = 2;
	}

 	static void
	default_callback(const size_t iter, void *, const gsl_multifit_nlinear_workspace *w)
	{
		if (iter == 1 or iter % 5 == 0)
		{
			gsl_vector *f = gsl_multifit_nlinear_residual(w);
			gsl_vector *x = gsl_multifit_nlinear_position(w);

			std::stringstream out;

			out << "iter " << iter;
			
			for (size_t i = 0 ; i < sizeof...(T) ; ++i)
			{
				out << ", " << i << ": ";
        		out << gsl_vector_get(x, i);
      		}
			
			out << ", |f(x)| = " << gsl_blas_dnrm2(f);

			BOOST_LOG_TRIVIAL(info) << out.str();
		}
	}

	static
    int
    fit_func (const gsl_vector* x, void* data, gsl_vector* f) {
      	User_data userdata = *static_cast<User_data*>(data);

  		std::apply([&x](auto&... elems){
			size_t i{0};
			((elems = gsl_vector_get (x, i), ++i), ...);
    	}, *userdata.free_variables);

		userdata.result->context_->apply_physics();
    	userdata.result->calculate();

		auto res = userdata.result->get_values();

    	for (size_t j = 0; j < userdata.fitting_data_points->size(); j++)
    	{
    	    gsl_vector_set (f, j, ((*userdata.fitting_data_points)[j] -  res[j]) / (*userdata.fitting_data_points)[j]);
    	}

    	return GSL_SUCCESS;
    };

	int
	fit(const Time_series::value_type& fitting_data, ICS_result& driver, double wt_pow = 1.2)
	{
		// vector that holds guessed g(t) for each iteration
		gsl_vector *f;

		size_t n = fitting_data.size();

		double x_init[sizeof...(T)];
		double weights[n];

		User_data userdata
		{
			&fitting_data,
			&free_variables,
			&driver
		};

		std::apply([&x_init](auto&&... elems){
			size_t i{0};
			((x_init[i] = std::forward<decltype(elems)>(elems), ++i), ...);
    	}, std::forward<std::tuple<T&...>>(free_variables));

		gsl_vector_view x = gsl_vector_view_array(x_init, sizeof...(T));

		for (size_t i = 0 ; i < n ; ++i)
		{
			double wt = fitting_data[i];
			weights[i] = std::pow(wt, wt_pow);
		}

		gsl_vector_view wts = gsl_vector_view_array(weights, n);

		double chisq, chisq0;
		int status, info;

		const double xtol = 1e-14;
		const double gtol = 1e-14;
		const double ftol = 1e-14;

		/* define the function to be minimized */
   	    function.f = &fit_func;
		function.df = nullptr;
		function.fvv = nullptr; // Voor geodesic aanzetten
		function.n = n;
		function.p = sizeof...(T);
		function.params = &userdata;

		/* allocate workspace with default parameters */
		workspace = gsl_multifit_nlinear_alloc(algorithm, &fdf_params, n, sizeof...(T));

		if (workspace == nullptr)
		{
			throw std::runtime_error("Could not allocate fitting solver: likely out of memory.");
		}

		/* initialize solver with starting point and weights */
		gsl_multifit_nlinear_winit(&x.vector, &wts.vector, &function, workspace);

		/* compute initial cost function */
		f = gsl_multifit_nlinear_residual(workspace);
		gsl_blas_ddot(f, f, &chisq0);

		status = gsl_multifit_nlinear_driver(100, xtol, gtol, ftol, callback_func, callback_params, &info, workspace);

		/* compute final cost */
	  	gsl_blas_ddot(f, f, &chisq);
		gsl_multifit_nlinear_free(workspace);

		if (status == GSL_SUCCESS)
		{
			BOOST_LOG_TRIVIAL(info) << "Fit converged.";
		}
		else if (status == GSL_EMAXITER)
		{
			throw std::runtime_error("Max iterations reached before converging.");
		}
		else if (status == GSL_ENOPROG)
		{
			throw std::runtime_error("Convergence too slow, exiting.");
		}
		return 0;
	}
};