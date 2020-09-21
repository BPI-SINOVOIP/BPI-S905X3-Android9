/*
 * Library:  lmfit (Levenberg-Marquardt least squares fitting)
 *
 * File:     demo/curve1.c
 *
 * Contents: Example for the solution of 2 nonlinear equations in 2 variables.
 *           Find the intersection of a circle and a parabola.
 *
 * Note:     Any modification of this example should be copied to the wiki.
 *
 * Author:   Joachim Wuttke <j.wuttke@fz-juelich.de> 2013
 * 
 * Licence:  see ../COPYING (FreeBSD)
 * 
 * Homepage: apps.jcns.fz-juelich.de/lmfit
 */

#include "lmmin.h"
#include <stdio.h>
#include <stdlib.h>

void evaluate_nonlin1(
    const double *p, int n, const void *data, double *f, int *info )
{
    f[0] = p[0]*p[0] + p[1]*p[1] - 1; /* unit circle    x^2+y^2=1 */
    f[1] = p[1] - p[0]*p[0];          /* standard parabola  y=x^2 */
}


int main( int argc, char **argv )
{
    int n = 2;   /* dimension of the problem */
    double p[2]; /* parameter vector p=(x,y) */

    /* auxiliary parameters */
    lm_control_struct control = lm_control_double;
    lm_status_struct  status;
    control.verbosity  = 31;

    /* get start values from command line */
    if( argc!=3 ){
        fprintf( stderr, "usage: nonlin1 x_start y_start\n" );
        exit(-1);
    }
    p[0] = atof( argv[1] );
    p[1] = atof( argv[2] );

    /* the minimization */
    printf( "Minimization:\n" );
    lmmin( n, p, n, NULL, evaluate_nonlin1, &control, &status );

    /* print results */
    printf( "\n" );
    printf( "lmmin status after %d function evaluations:\n  %s\n",
            status.nfev, lm_infmsg[status.outcome] );

    printf( "\n" );
    printf("Solution:\n");
    printf("  x = %19.11f\n", p[0]);
    printf("  y = %19.11f\n", p[1]);
    printf("  d = %19.11f => ", status.fnorm);

    /* convergence of lmfit is not enough to ensure validity of the solution */
    if( status.fnorm >= control.ftol )
        printf( "not a valid solution, try other starting values\n" );
    else
        printf( "valid, though not the only solution: "
                "try other starting values\n" );

    return 0;
}
