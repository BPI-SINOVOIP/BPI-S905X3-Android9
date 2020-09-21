/*
 * Library:  lmfit (Levenberg-Marquardt least squares fitting)
 *
 * File:     demo/curve1.c
 *
 * Contents: Example for fitting data with error bars:
 *           fit a data set (x,y+-dy) by a curve f(x;p).
 *
 * Author:   Joachim Wuttke <j.wuttke@fz-juelich.de> 2004-2013
 *
 * Licence:  see ../COPYING (FreeBSD)
 *
 * Homepage: apps.jcns.fz-juelich.de/lmfit
 */

#include "lmcurve_tyd.h"
#include <stdio.h>

/* model function: a parabola */

double f( double t, const double *p )
{
    return p[0] + p[1]*t + p[2]*t*t;
}

int main()
{
    int n = 3; /* number of parameters in model function f */
    double par[3] = { 100, 0, -10 }; /* really bad starting value */

    /* data points: a slightly distorted standard parabola */
    int m = 9;
    int i;
    double t[9] = { -4., -3., -2., -1.,  0., 1.,  2.,  3.,  4. };
    double y[9] = { 16.6, 9.9, 4.4, 1.1, 0., 1.1, 4.2, 9.3, 16.4 };
    double dy[9] = { 4, 3, 2, 1, 2, 3, 4, 5, 6 };

    lm_control_struct control = lm_control_double;
    lm_status_struct status;
    control.verbosity = 1;

    printf( "Fitting ...\n" );
    /* now the call to lmfit */
    lmcurve_tyd( n, par, m, t, y, dy, f, &control, &status );

    printf( "Results:\n" );
    printf( "status after %d function evaluations:\n  %s\n",
            status.nfev, lm_infmsg[status.outcome] );

    printf("obtained parameters:\n");
    for ( i = 0; i < n; ++i)
        printf("  par[%i] = %12g\n", i, par[i]);
    printf("obtained norm:\n  %12g\n", status.fnorm );

    printf("fitting data as follows:\n");
    for ( i = 0; i < m; ++i)
        printf(
        "  t[%1d]=%2g y=%5.1f+-%4.1f fit=%8.5f residue=%8.4f weighed=%8.4f\n",
        i, t[i], y[i], dy[i], f(t[i],par), y[i] - f(t[i],par),
        (y[i] - f(t[i],par))/dy[i] );

    return 0;
}
