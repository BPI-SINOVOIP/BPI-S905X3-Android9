/*
 * Library:  lmfit (Levenberg-Marquardt least squares fitting)
 *
 * File:     demo/curve1.c
 *
 * Contents: Example for curve fitting with lmcurve():
 *           fit a data set y(x) by a curve f(x;p).
 *
 * Note:     Any modification of this example should be copied to
 *           the manual page source lmcurve.pod and to the wiki.
 *
 * Author:   Joachim Wuttke <j.wuttke@fz-juelich.de> 2004-2013
 *
 * Licence:  see ../COPYING (FreeBSD)
 *
 * Homepage: apps.jcns.fz-juelich.de/lmfit
 */

#include "lmcurve.h"
#include <stdio.h>
#include <stdlib.h>

void lm_qrfac(int m, int n, double *a, int *ipvt,
              double *rdiag, double *acnorm, double *wa);

void set_orthogonal( int n, double *Q, double* v )
{
    int i, j;
    double temp = 0;
    for (i=0; i<n; ++i)
        temp += v[i]*v[i];
    for (i=0; i<n; ++i)
        for (j=0; j<n; ++j)
            Q[j*n+i] = ( i==j ? 1 : 0 ) - 2*v[i]*v[j]/temp;
}

void matrix_multiplication( int n, double *A, double *Q, double *R )
{
    int i,j,k;
    double temp;
    for (i=0; i<n; ++i) {
        for (j=0; j<n; ++j) {
            temp = 0;
            for (k=0; k<n; ++k) {
                temp += Q[k*n+i]*R[j*n+k];
            }
            A[j*n+i] = temp;
        }
    }
}

void matrix_transposition( int n, double *A )
{
    int i,j;
    double temp;
    for (i=0; i<n; ++i) {
        for (j=i+1; j<n; ++j) {
            temp = A[j*n+i];
            A[j*n+i] = A[i*n+j];
            A[i*n+j] = temp;
        }
    }
}

void print_matrix(int m, int n, double *a)
{
    int i,j;
    for (i=0; i<m; ++i) {
        for (j=0; j<n; ++j) {
            printf( "%8.4g ", a[j*m+i] );
        }
        printf ("\n");
    }
}

int main( int argc, char *argv[] )
{
    double A[9], rdiag[3], acnorm[3], wa[3];
    int i, ipvt[3];

    if ( argc!= 10 ) {
        fprintf( stderr, "bad # args\n" );
        exit(1);
    }
    for ( int i=0; i<9; ++i )
        A[i] = atof( argv[1+i] );
    matrix_transposition( 3, A );

    printf( "Input matrix A:\n" );
    print_matrix(3, 3, A);

    lm_qrfac(3, 3, A, ipvt, rdiag, acnorm, wa);

    printf( "Output matrix A:\n" );
    print_matrix(3, 3, A);

    printf( "Output vector rdiag:\n" );
    print_matrix(1, 3, rdiag);

    printf( "Output vector acnorm:\n" );
    print_matrix(1, 3, acnorm);

    printf( "Output vector ipvt:\n" );
    for (i=0; i<3; ++i)
        printf( "%i ", ipvt[i] );
    printf("\n");
}
