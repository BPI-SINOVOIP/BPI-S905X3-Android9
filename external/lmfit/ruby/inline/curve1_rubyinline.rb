#! /usr/bin/env ruby

# This is 'curve1.c' demo done with RubyInline gem (instead of using swig)
# Contributed by Igor Drozdov <idrozdov@gmail.com> 2012
# $ ruby curve1_rubyinline.rb
# 
# Requirements:
# 
# $ gem install RubyInline
#
# To make sure header files and shared librares can be found,
# export appropriate LD_LIBRARY_PATH and C_INCLUDE_PATH.

require 'rubygems'
require 'inline'

class Curve
  inline(:C) do |builder|
    builder.include("\"lmcurve.h\"")
    builder.include("\"lmmin.h\"")
    builder.include("<math.h>")
    builder.include("<stdio.h>")
    builder.include("<ruby.h>)")
    builder.add_link_flags("-llmmin")

    builder.c_raw <<EOC
double f( double t, const double *p )
{
    return p[0] + p[1]*(t-p[2])*(t-p[2]);
}
EOC

    builder.c <<EOC
VALUE demo(VALUE t_ary, VALUE y_ary) {
    /* parameter vector */
    int n_par = 3; // number of parameters in model function f
    double par[3] = { 1, 0, -1 }; // relatively bad starting value

    /* data pairs: slightly distorted standard parabola */

    int m_dat = 11; // number of data pairs
    int i;

    /* ruby macros dealing with inputs */
    int t_len = RARRAY_LEN(t_ary);
    int y_len = RARRAY_LEN(y_ary);
    
    double t[t_len];
    double y[t_len];

    VALUE *t_arr = RARRAY_PTR(t_ary);
    VALUE *y_arr = RARRAY_PTR(y_ary);

    VALUE result_pars = rb_ary_new();   // ruby array containing fitted params

    /* auxiliary parameters */
    lm_status_struct status;
    lm_control_struct control = lm_control_double;

    for (i = 0; i < t_len; i++) {	
      t[i] = NUM2DBL(t_arr[i]);
      y[i] = NUM2DBL(y_arr[i]);
    }

    lmcurve_fit( n_par, par, m_dat, t, y, f, &control, &status );
    /* print results */

    printf( "\\nResults:\\n" );
    printf( "status after %d function evaluations:\\n  %s\\n",
            status.nfev, lm_infmsg[status.outcome] );

    printf("obtained parameters:\\n");
    for ( i = 0; i < n_par; ++i)
         rb_ary_push(result_pars,DBL2NUM(par[i]));
 	printf("  par[%i] = %12g\\n", i, par[i]);
    printf("obtained norm:\\n  %12g\\n", status.fnorm );

    printf("fitting data as follows:\\n");
    for ( i = 0; i < m_dat; ++i)
        printf( "  t[%2d]=%12g y=%12g fit=%12g residue=%12g\\n",
                i, t[i], y[i], f(t[i],par), y[i] - f(t[i],par) );

    printf("\\n");
    return result_pars;
}
EOC

  end
end

# 

t = [ -5.0, -4.0, -3.0, -2.0, -1.0, 0.0, 1.0, 2.0, 3.0, 4.0, 5.0 ]
y = [ 25.5, 16.6, 9.9, 4.4, 1.1, 0, 1.1, 4.2, 9.3, 16.4, 25.5 ]

c = Curve.new
p c.demo(t,y)
