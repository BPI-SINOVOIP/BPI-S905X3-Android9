/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "TradeFedNativeTestSampleLib.h"

namespace TFNativeTestSampleLib {

// Calculate fibonacci recursively, where 0 <= n <= 47
unsigned long fibonacci_recursive(int n) {
    if (n == 0) {
        return 0;
    }
    else if (n == 1) {
        return 1;
    }
    else {
        return fibonacci_recursive(n-1) + fibonacci_recursive(n-2);
    }
}

// Calculate fibonacci iteratively, where 0 <= n <= 47
unsigned long fibonacci_iterative(int n) {
    unsigned long n1 = 1;
    unsigned long n2 = 0;

    if (n == 0) {
        return 0;
    }
    else if (n == 1) {
        return 1;
    }
    for (int i=1; i<n; ++i) {
        unsigned long current = n1 + n2;
        n2 = n1;
        n1 = current;
    }
    return n1;
}

// Converts Celcius to Farenheit
double c_to_f(double celcius) {
    return ((celcius * 9) / 5) + 32.0;
}

// Converts Farenheit to Celcius
double f_to_c(double farenheit) {
    return ((farenheit - 32.0) * 5) / 9;
}
};
