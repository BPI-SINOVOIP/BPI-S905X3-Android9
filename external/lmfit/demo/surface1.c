/*
 * Library:  lmfit (Levenberg-Marquardt least squares fitting)
 *
 * File:     surface1.c
 *
 * Contents: Example for generic minimization with lmmin():
             fit data y(t) by a function f(t;p), where t is a 2-vector.
 *
 * Note:     Any modification of this example should be copied to
 *           the manual page source lmmin.pod and to the wiki.
 *
 * Author:   Joachim Wuttke 2010, following a suggestion by Mario Rudolphi
 * 
 * Licence:  see ../COPYING (FreeBSD)
 * 
 * Homepage: apps.jcns.fz-juelich.de/lmfit
 */
 
#include "lmmin.h"
#include <stdio.h>

/* fit model: a plane p0 + p1*tx + p2*tz */
double f( double tx, double tz, const double *p )
{
    return p[0] + p[1]*tx + p[2]*tz;
}

/* data structure to transmit arrays and fit model */
typedef struct {
    double *tx, *tz;
    double *y;
    double (*f)( double tx, double tz, const double *p );
} data_struct;

/* function evaluation, determination of residues */
void evaluate_surface( const double *par, int m_dat, const void *data,
                       double *fvec, int *info )
{
    /* for readability, explicit type conversion */
    data_struct *D;
    D = (data_struct*)data;

    int i;
    for ( i = 0; i < m_dat; i++ )
	fvec[i] = D->y[i] - D->f( D->tx[i], D->tz[i], par );
}

int main()
{
    /* parameter vector */
    int n_par = 3;                /* number of parameters in model function f */
    double par[3] = { -1, 0, 1 }; /* arbitrary starting value */

    /* data points */
    int m_dat = 4;
    double tx[4] = { -1, -1,  1,  1 };
    double tz[4] = { -1,  1, -1,  1 };
    double y[4]  = {  0,  1,  1,  2 };

    data_struct data = { tx, tz, y, f };

    /* auxiliary parameters */
    lm_status_struct status;
    lm_control_struct control = lm_control_double;
    control.verbosity = 9;

    /* perform the fit */
    printf( "Fitting:\n" );
    lmmin( n_par, par, m_dat, (const void*) &data,
           evaluate_surface, &control, &status );

    /* print results */
    printf( "\nResults:\n" );
    printf( "status after %d function evaluations:\n  %s\n",
            status.nfev, lm_infmsg[status.outcome] );

    printf("obtained parameters:\n");
    int i;
    for ( i=0; i<n_par; ++i )
	printf("  par[%i] = %12g\n", i, par[i]);
    printf("obtained norm:\n  %12g\n", status.fnorm );

    printf("fitting data as follows:\n");
    double ff;
    for ( i=0; i<m_dat; ++i ){
        ff = f(tx[i], tz[i], par);
        printf( "  t[%2d]=%12g,%12g y=%12g fit=%12g residue=%12g\n",
                i, tx[i], tz[i], y[i], ff, y[i] - ff );
    }

    return 0;
}
