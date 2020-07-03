#ifndef FIT_H
#define FIT_H

#include <gsl/gsl_vector.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_multifit_nlinear.h>

#include <vector>
#include <functional>
#include <memory>

namespace fit
{

template<typename Engine_t>
struct data {
  size_t n;
  double * g_t;
  Engine_t* engine;
};

}

void callback(const size_t iter, void *, const gsl_multifit_nlinear_workspace *w);

template<typename Engine_t>
int
run_test_fit (int(*exp_bf)(const gsl_vector*, void*, gsl_vector*), const Engine_t& engine, std::vector<double> g_t)
{
  const gsl_multifit_nlinear_type *T = gsl_multifit_nlinear_trust;
  gsl_multifit_nlinear_workspace *w;
  gsl_multifit_nlinear_fdf fdf;
  gsl_multifit_nlinear_parameters fdf_params = gsl_multifit_nlinear_default_parameters();
  fdf_params.fdtype = GSL_MULTIFIT_NLINEAR_CTRDIFF;
  
  const size_t n = g_t.size();
  const size_t p = 2;

  gsl_vector *f;
  gsl_matrix *J;
  gsl_matrix *covar = gsl_matrix_alloc (p, p);
  struct fit::data<const Engine_t> d = { n, g_t.data(), &engine};
    
  double x_init[2] = { 150.0, 1.2, }; /* starting values */
  gsl_vector_view x = gsl_vector_view_array (x_init, p);
  double chisq, chisq0;
  int status, info;

  const double xtol = 1e-8;
  const double gtol = 1e-8;
  const double ftol = 0.0;

  /* define the function to be minimized */
  fdf.f = exp_bf;
  fdf.df = nullptr;   /* set to NULL for finite-difference Jacobian */
  fdf.fvv = nullptr;     /* not using geodesic acceleration */
  fdf.n = n;
  fdf.p = p;
  fdf.params = &d;

  /* allocate workspace with default parameters */
  w = gsl_multifit_nlinear_alloc (T, &fdf_params, n, p);

  /* initialize solver with starting point and weights */
  gsl_multifit_nlinear_init (&x.vector, &fdf, w);

  /* compute initial cost function */
  f = gsl_multifit_nlinear_residual(w);
  gsl_blas_ddot(f, f, &chisq0);

  /* solve the system with a maximum of 100 iterations */
  status = gsl_multifit_nlinear_driver(100, xtol, gtol, ftol, callback, NULL, &info, w);

  /* compute covariance of best fit parameters */
  J = gsl_multifit_nlinear_jac(w);
  gsl_multifit_nlinear_covar (J, 0.0, covar);

  /* compute final cost */
  gsl_blas_ddot(f, f, &chisq);

#define FIT(i) gsl_vector_get(w->x, i)
#define ERR(i) sqrt(gsl_matrix_get(covar,i,i))

  fprintf(stderr, "summary from method '%s/%s'\n",
          gsl_multifit_nlinear_name(w),
          gsl_multifit_nlinear_trs_name(w));
  fprintf(stderr, "number of iterations: %zu\n",
          gsl_multifit_nlinear_niter(w));
  fprintf(stderr, "function evaluations: %zu\n", fdf.nevalf);
  fprintf(stderr, "Jacobian evaluations: %zu\n", fdf.nevaldf);
  fprintf(stderr, "reason for stopping: %s\n",
          (info == 1) ? "small step size" : "small gradient");
  fprintf(stderr, "initial |f(x)| = %f\n", sqrt(chisq0));
  fprintf(stderr, "final   |f(x)| = %f\n", sqrt(chisq));

  {
    double dof = n - p;
    double c = GSL_MAX_DBL(1, sqrt(chisq / dof));

    fprintf(stderr, "chisq/dof = %g\n", chisq / dof);

    fprintf (stderr, "tau_e      = %.5f +/- %.5f\n", FIT(0), c*ERR(0));
    fprintf (stderr, "G_e = %.5f +/- %.5f\n", FIT(1), c*ERR(1));
  }

  fprintf (stderr, "status = %s\n", gsl_strerror (status));

  gsl_multifit_nlinear_free (w);
  gsl_matrix_free (covar);

  return 0;
}

#endif