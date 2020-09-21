# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

fortify-test-src := clang-fortify-tests.cpp
# -U_FORTIFY_SOURCE: Clang gets unhappy being passed `-D_FORTIFY_SOURCE=2
#  -D_FORTIFY_SOURCE=1` (the first of which comes from our wrapper).
fortify-cxx := $(CXX) $(fortify-test-src) $(CPPFLAGS) $(CXXFLAGS) -std=c++11 \
	-D_GNU_SOURCE -fno-exceptions -O2 -U_FORTIFY_SOURCE
fortify-diag-flags := -o /dev/null -Xclang -verify -DCOMPILATION_TESTS -c
fortify-runtime-flags := -Wno-user-defined-warnings -Wno-unused-result

all: fortify-runtime-tests

fortify-runtime-tests: clang-fortify-driver.o clang-fortify-tests-1.o \
                       clang-fortify-tests-2.o
	$(CXX) $(LDFLAGS) -o $@ $+

clang-fortify-tests-1.o: $(fortify-test-src)
	$(fortify-cxx) $(fortify-runtime-flags) -c -D_FORTIFY_SOURCE=1 -o $@

clang-fortify-tests-2.o: $(fortify-test-src)
	$(fortify-cxx) $(fortify-runtime-flags) -c -D_FORTIFY_SOURCE=2 -o $@

clean:
	rm -f clang-fortify-*.o fortify-runtime-tests

install:
	install -m 0755 -d $(DESTDIR)/usr/local/bin
	install -m 0755 toolchain-tests $(DESTDIR)/usr/local/bin
	install -m 0755 fortify-runtime-tests $(DESTDIR)/usr/local/bin

.PHONY: check
check:
	$(fortify-cxx) $(fortify-diag-flags) -D_FORTIFY_SOURCE=1
	$(fortify-cxx) $(fortify-diag-flags) -D_FORTIFY_SOURCE=2
