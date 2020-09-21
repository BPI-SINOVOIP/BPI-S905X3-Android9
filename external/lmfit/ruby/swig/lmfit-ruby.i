%module lmfit

%{
#include <lmmin.h>
#include <lmcurve.h>
%}

%include "cpointer.i"
%include "carrays.i"

%inline %{
extern const lm_control_struct lm_control_float;
extern const lm_control_struct lm_control_double;
%}

%pointer_functions(unsigned short, usp)
%array_functions(double, doubleArray);

double lm_enorm( int, const double * );

double lm_enorm( int, const double * );

void lmcurve_fit( int, double*, int, const double*, const double*,
                  double (*f)( double, const double *),
                  lm_control_struct*, lm_status_struct* );
