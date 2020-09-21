// RUN: %clang_esan_wset -O0 %s -o %t 2>&1
// RUN: %run %t 2>&1 | FileCheck %s --check-prefix=CHECK-ESAN

// RUN: %clang -O0 %s -o %t 2>&1
// RUN: %run %t 2>&1 | FileCheck %s --check-prefix=CHECK-NO-ESAN

#include <sanitizer/esan_interface.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

const int size = 0x1 << 25; // 523288 cache lines
const int iters = 6;

int main(int argc, char **argv) {
  char *buf = (char *)mmap(0, size, PROT_READ | PROT_WRITE,
                           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  // Try to increase the probability that the sideline thread is
  // scheduled.  Unfortunately we can't do proper synchronization
  // without some form of annotation or something.
  sched_yield();
  // Do enough work to get at least 4 samples.
  for (int j = 0; j < iters; ++j) {
    for (int i = 0; i < size; ++i)
      buf[i] = i;
    sched_yield();
  }
  // Ensure a non-esan build works without ifdefs:
  if (__esan_report) {
    // We should get 2 roughly identical reports:
    __esan_report();
  }
  munmap(buf, size);
  fprintf(stderr, "all done\n");
  // CHECK-NO-ESAN:   all done
  // We only check for a few samples here to reduce the chance of flakiness:
  // CHECK-ESAN:      =={{[0-9]+}}== Total number of samples: {{[0-9]+}}
  // CHECK-ESAN-NEXT: =={{[0-9]+}}== Samples array #0 at period 20 ms
  // CHECK-ESAN-NEXT: =={{[0-9]+}}==#   0: {{[ 0-9]+}} {{KB|MB|Bytes}} ({{[ 0-9]+}} cache lines)
  // CHECK-ESAN-NEXT: =={{[0-9]+}}==#   1: {{[ 0-9]+}} {{KB|MB|Bytes}} ({{[ 0-9]+}} cache lines)
  // CHECK-ESAN-NEXT: =={{[0-9]+}}==#   2: {{[ 0-9]+}} {{KB|MB|Bytes}} ({{[ 0-9]+}} cache lines)
  // CHECK-ESAN-NEXT: =={{[0-9]+}}==#   3: {{[ 0-9]+}} {{KB|MB|Bytes}} ({{[ 0-9]+}} cache lines)
  // CHECK-ESAN:      =={{[0-9]+}}== Samples array #1 at period 80 ms
  // CHECK-ESAN-NEXT: =={{[0-9]+}}==#   0: {{[ 0-9]+}} {{KB|MB|Bytes}} ({{[ 0-9]+}} cache lines)
  // CHECK-ESAN:      =={{[0-9]+}}== Samples array #2 at period 320 ms
  // CHECK-ESAN:      =={{[0-9]+}}== Samples array #3 at period 1280 ms
  // CHECK-ESAN:      =={{[0-9]+}}== Samples array #4 at period 5120 ms
  // CHECK-ESAN:      =={{[0-9]+}}== Samples array #5 at period 20 sec
  // CHECK-ESAN:      =={{[0-9]+}}== Samples array #6 at period 81 sec
  // CHECK-ESAN:      =={{[0-9]+}}== Samples array #7 at period 327 sec
  // CHECK-ESAN: {{.*}} EfficiencySanitizer: the total working set size: 32 MB (5242{{[0-9][0-9]}} cache lines)
  // CHECK-ESAN-NEXT: all done
  // CHECK-ESAN-NEXT: =={{[0-9]+}}== Total number of samples: {{[0-9]+}}
  // CHECK-ESAN-NEXT: =={{[0-9]+}}== Samples array #0 at period 20 ms
  // CHECK-ESAN-NEXT: =={{[0-9]+}}==#   0: {{[ 0-9]+}} {{KB|MB|Bytes}} ({{[ 0-9]+}} cache lines)
  // CHECK-ESAN-NEXT: =={{[0-9]+}}==#   1: {{[ 0-9]+}} {{KB|MB|Bytes}} ({{[ 0-9]+}} cache lines)
  // CHECK-ESAN-NEXT: =={{[0-9]+}}==#   2: {{[ 0-9]+}} {{KB|MB|Bytes}} ({{[ 0-9]+}} cache lines)
  // CHECK-ESAN-NEXT: =={{[0-9]+}}==#   3: {{[ 0-9]+}} {{KB|MB|Bytes}} ({{[ 0-9]+}} cache lines)
  // CHECK-ESAN:      =={{[0-9]+}}== Samples array #1 at period 80 ms
  // CHECK-ESAN-NEXT: =={{[0-9]+}}==#   0: {{[ 0-9]+}} {{KB|MB|Bytes}} ({{[ 0-9]+}} cache lines)
  // CHECK-ESAN:      =={{[0-9]+}}== Samples array #2 at period 320 ms
  // CHECK-ESAN:      =={{[0-9]+}}== Samples array #3 at period 1280 ms
  // CHECK-ESAN:      =={{[0-9]+}}== Samples array #4 at period 5120 ms
  // CHECK-ESAN:      =={{[0-9]+}}== Samples array #5 at period 20 sec
  // CHECK-ESAN:      =={{[0-9]+}}== Samples array #6 at period 81 sec
  // CHECK-ESAN:      =={{[0-9]+}}== Samples array #7 at period 327 sec
  // CHECK-ESAN: {{.*}} EfficiencySanitizer: the total working set size: 32 MB (5242{{[0-9][0-9]}} cache lines)
  return 0;
}
