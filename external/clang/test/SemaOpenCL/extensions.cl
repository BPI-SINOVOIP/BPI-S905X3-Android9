// RUN: %clang_cc1 %s -triple spir-unknown-unknown -verify -pedantic -fsyntax-only
// RUN: %clang_cc1 %s -triple spir-unknown-unknown -verify -pedantic -fsyntax-only -cl-std=CL1.1

// Test with a target not supporting fp64.
// RUN: %clang_cc1 %s -triple r600-unknown-unknown -target-cpu r600 -verify -pedantic -fsyntax-only -DNOFP64

void f1(double da) { // expected-error {{type 'double' requires cl_khr_fp64 extension}}
  double d; // expected-error {{type 'double' requires cl_khr_fp64 extension}}
  (void) 1.0; // expected-warning {{double precision constant requires cl_khr_fp64}}
}

#pragma OPENCL EXTENSION cl_khr_fp64 : enable
#ifdef NOFP64
// expected-warning@-2{{unsupported OpenCL extension 'cl_khr_fp64' - ignoring}}
#endif

void f2(void) {
  double d;
#ifdef NOFP64
// expected-error@-2{{use of type 'double' requires cl_khr_fp64 extension to be enabled}}
#endif

  (void) 1.0;
#ifdef NOFP64
// expected-warning@-2{{double precision constant requires cl_khr_fp64, casting to single precision}}
#endif
}

#pragma OPENCL EXTENSION cl_khr_fp64 : disable
#ifdef NOFP64
// expected-warning@-2{{unsupported OpenCL extension 'cl_khr_fp64' - ignoring}}
#endif

void f3(void) {
  double d; // expected-error {{type 'double' requires cl_khr_fp64 extension}}
}
