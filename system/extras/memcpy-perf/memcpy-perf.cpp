#include <iostream>
#include <chrono>
#include <vector>
#include <algorithm>
#include <numeric>
#include <stdlib.h>
#include <memory>
#include <cmath>
#include <string>
#include <thread>

#define CACHE_HIT_SIZE 1 << 17

using namespace std;

size_t size_start = 64;
size_t size_end = 16 * (1ull << 20);
size_t samples = 2048;
size_t size_per_test = 64 * (1ull << 20);
size_t tot_sum = 0;
size_t delay = 0;
float speed = 0;
bool dummy = false;

void __attribute__((noinline)) memcpy_noinline(void *dst, void *src, size_t size);
void __attribute__((noinline)) memset_noinline(void *dst, int value, size_t size);
uint64_t __attribute__((noinline)) sum(volatile void *src, size_t size);

enum BenchType {
    MemcpyBench,
    MemsetBench,
    SumBench,
};

static void usage(char* p) {
    printf("Usage: %s <test> <options>\n"
           "<test> is one of the following:\n"
           "  --memcpy\n"
           "  --memset\n"
           "  --sum\n"
           "<options> are optional and apply to all tests:\n"
           "  --dummy\n"
           "    Simulates cpu-only load of a test. Guaranteed to use L2\n"
           "    instead.  Not supported on --sum test.\n"
           "  --delay DELAY_DIVISOR\n"
           "  --start START_SIZE_MB\n"
           "    --end END_SIZE_MB (requires start, optional)\n"
           "  --samples NUM_SAMPLES\n"
           , p);
}

int main(int argc, char *argv[])
{
    BenchType type = MemcpyBench;
    if (argc <= 1) {
        usage(argv[0]);
        return 0;
    }
    for (int i = 1; i < argc; i++) {
      if (string(argv[i]) == string("--memcpy")) {
         type = MemcpyBench;
      } else if (string(argv[i]) == string("--memset")) {
         type = MemsetBench;
      } else if (string(argv[i]) == string("--sum")) {
         type = SumBench;
      } else if (string(argv[i]) == string("--dummy")) {
         dummy = true;
      } else if (i + 1 < argc) {
          if (string(argv[i]) == string("--delay")) {
             delay = atoi(argv[++i]);
          } else if (string(argv[i]) == string("--start")) {
             size_start = atoi(argv[++i]) * (1ull << 20);
             size_end = size_start;
          } else if (string(argv[i]) == string("--end")) {
             size_t end = atoi(argv[++i]) * (1ull << 20);
             if (end > size_start && i > 3
                 && string(argv[i-3]) == string("--start")) {
                 size_end = end;
             } else {
                 printf("Cannot specify --end without --start.\n");
                 return 0;
             }
          } else if (string(argv[i]) == string("--samples")) {
             samples = atoi(argv[++i]);
          } else {
             printf("Unknown argument %s\n", argv[i]);
             return 0;
          }
       } else {
          printf("The %s option requires a single argument.\n", argv[i]);
          return 0;
       }
    }

    unique_ptr<uint8_t[]> src(new uint8_t[size_end]);
    unique_ptr<uint8_t[]> dst(new uint8_t[size_end]);
    memset(src.get(), 1, size_end);

    double start_pow = log10(size_start);
    double end_pow = log10(size_end);
    double pow_inc = (end_pow - start_pow) / samples;

    //cout << "src: " << (uintptr_t)src.get() << endl;
    //cout << "dst: " <<  (uintptr_t)dst.get() << endl;

    for (double cur_pow = start_pow; cur_pow <= end_pow && samples > 0;
            cur_pow += pow_inc) {
        chrono::time_point<chrono::high_resolution_clock>
            copy_start, copy_end;

        size_t cur_size = (size_t)pow(10.0, cur_pow);
        size_t iter_per_size = size_per_test / cur_size;

        // run benchmark
        switch (type) {
            case MemsetBench: {
                memcpy_noinline(src.get(), dst.get(), cur_size);
                memset_noinline(dst.get(), 0xdeadbeef, cur_size);
                size_t hit_size = CACHE_HIT_SIZE;
                copy_start = chrono::high_resolution_clock::now();
                for (int i = 0; i < iter_per_size; i++) {
                    if (!dummy) {
                        memset_noinline(dst.get(), 0xdeadbeef, cur_size);
                    } else {
                        while (hit_size < cur_size) {
                            memset_noinline
                                (dst.get(), 0xdeadbeef, CACHE_HIT_SIZE);
                            hit_size += 1 << 17;
                        }
                    }
                    if (delay != 0)
                        this_thread::sleep_for(chrono
                            ::nanoseconds(size_per_test / delay));
                }
                copy_end = chrono::high_resolution_clock::now();
                break;
            }
            case MemcpyBench: {
                memcpy_noinline(dst.get(), src.get(), cur_size);
                memcpy_noinline(src.get(), dst.get(), cur_size);
                size_t hit_size = CACHE_HIT_SIZE;
                copy_start = chrono::high_resolution_clock::now();
                for (int i = 0; i < iter_per_size; i++) {
                    if (!dummy) {
                        memcpy_noinline(dst.get(), src.get(), cur_size);
                    } else {
                        while (hit_size < cur_size) {
                            memcpy_noinline
                                (dst.get(), src.get(), CACHE_HIT_SIZE);
                            hit_size += CACHE_HIT_SIZE;
                        }
                    }
                    if (delay != 0)
                        this_thread::sleep_for(chrono
                            ::nanoseconds(size_per_test / delay));
                }
                copy_end = chrono::high_resolution_clock::now();
                break;
            }
            case SumBench: {
                uint64_t s = 0;
                s += sum(src.get(), cur_size);
                copy_start = chrono::high_resolution_clock::now();
                for (int i = 0; i < iter_per_size; i++) {
                    s += sum(src.get(), cur_size);
                    if (delay != 0)
                        this_thread::sleep_for(chrono
                            ::nanoseconds(size_per_test / delay));
                }
                copy_end = chrono::high_resolution_clock::now();
                tot_sum += s;
                break;
            }
        }

        samples--;
        double ns_per_copy = chrono::duration_cast<chrono::nanoseconds>(copy_end - copy_start).count() / double(iter_per_size);
        double gb_per_sec = ((double)cur_size / (1ull<<30)) / (ns_per_copy / 1.0E9);
        if (type == MemcpyBench)
            gb_per_sec *= 2.0;
        double percent_waiting = 0;
        if (delay != 0) {
            percent_waiting = (size_per_test / delay) / ns_per_copy * 100;
        }
        cout << "size: " << cur_size << ", perf: " << gb_per_sec
             << "GB/s, iter: " << iter_per_size << ", \% time spent waiting: "
             << percent_waiting << endl;
    }
    return 0;
}
