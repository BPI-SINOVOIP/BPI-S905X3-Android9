# Default options
USE_BSDIFF ?= y

EXECUTABLES-y := bspatch
LIBRARIES-y := libbspatch.so
EXECUTABLES-$(USE_BSDIFF) += bsdiff
LIBRARIES-$(USE_BSDIFF) += libbsdiff.so

BINARIES := $(EXECUTABLES-y) $(LIBRARIES-y)

INSTALL = install
CPPFLAGS += -Iinclude -I..
CXXFLAGS += -std=c++11 -O3 -Wall -Werror -fPIC

DESTDIR ?=
PREFIX = /usr
BINDIR = $(PREFIX)/bin
DATADIR = $(PREFIX)/share
MANDIR = $(DATADIR)/man
MAN1DIR = $(MANDIR)/man1
INCLUDEDIR ?= $(PREFIX)/include
GENTOO_LIBDIR ?= lib
LIBDIR ?= $(PREFIX)/$(GENTOO_LIBDIR)
INSTALL_PROGRAM ?= $(INSTALL) -c -m 755
INSTALL_MAN ?= $(INSTALL) -c -m 444

.PHONY: all test clean install
all: $(BINARIES)
test: bsdiff_unittest
clean:
	rm -f *.o $(BINARIES) bsdiff_unittest .deps

### List of source files for each project. Keep in sync with the Android.mk.
# "bsdiff" program.
bsdiff_src_files := \
    brotli_compressor.cc \
    bsdiff.cc \
    bz2_compressor.cc \
    compressor_buffer.cc \
    diff_encoder.cc \
    endsley_patch_writer.cc \
    logging.cc \
    patch_writer.cc \
    patch_writer_factory.cc \
    split_patch_writer.cc \
    suffix_array_index.cc

# "bspatch" program.
bspatch_src_files := \
    brotli_decompressor.cc \
    bspatch.cc \
    bz2_decompressor.cc \
    buffer_file.cc \
    decompressor_interface.cc \
    extents.cc \
    extents_file.cc \
    file.cc \
    logging.cc \
    memory_file.cc \
    patch_reader.cc \
    sink_file.cc \
    utils.cc

# Unit test files.
bsdiff_common_unittests := \
    brotli_compressor_unittest.cc \
    bsdiff_arguments.cc \
    bsdiff_arguments_unittest.cc \
    bsdiff_unittest.cc \
    bspatch_unittest.cc \
    diff_encoder_unittest.cc \
    endsley_patch_writer_unittest.cc \
    extents_file_unittest.cc \
    extents_unittest.cc \
    patch_reader_unittest.cc \
    patch_writer_unittest.cc \
    split_patch_writer_unittest.cc \
    suffix_array_index_unittest.cc \
    test_utils.cc \
    testrunner.cc


BSDIFF_LIBS := -lbz2 -lbrotlienc -ldivsufsort -ldivsufsort64
BSDIFF_OBJS := $(bsdiff_src_files:.cc=.o)
BSPATCH_LIBS := -lbz2 -lbrotlidec
BSPATCH_OBJS := $(bspatch_src_files:.cc=.o)

UNITTEST_LIBS = -lgmock -lgtest -lpthread
UNITTEST_OBJS := $(bsdiff_common_unittests:.cc=.o)

bsdiff: $(BSDIFF_OBJS) bsdiff_arguments.o bsdiff_main.o
bsdiff: LDLIBS += $(BSDIFF_LIBS)
libbsdiff.so: $(BSDIFF_OBJS)
libbsdiff.so: LDLIBS += $(BSDIFF_LIBS)

bspatch: $(BSPATCH_OBJS) bspatch_main.o
bspatch: LDLIBS += $(BSPATCH_LIBS)
libbspatch.so: $(BSPATCH_OBJS)
libbspatch.so: LDLIBS += $(BSPATCH_LIBS)

bsdiff_unittest: LDLIBS += $(BSDIFF_LIBS) $(BSPATCH_LIBS) $(UNITTEST_LIBS)
bsdiff_unittest: $(BSPATCH_OBJS) $(BSDIFF_OBJS) $(UNITTEST_OBJS)

bsdiff_unittest bsdiff bspatch:
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

libbsdiff.so libbspatch.so:
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -Wl,-soname,$@ -shared -o $@ $^ $(LDLIBS)

# Source file dependencies.
.deps: $(bsdiff_src_files) $(bspatch_src_files) $(bsdiff_common_unittests) \
       bsdiff_main.cc bspatch_main.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MM $^ >$@ || (rm -f $@; false)
-include .deps

install:
	mkdir -p $(DESTDIR)$(BINDIR) $(DESTDIR)$(LIBDIR) $(DESTDIR)$(MAN1DIR) \
	  $(DESTDIR)/$(INCLUDEDIR)/bsdiff
	$(INSTALL_PROGRAM) $(EXECUTABLES-y) $(DESTDIR)$(BINDIR)
	$(INSTALL_PROGRAM) $(LIBRARIES-y) $(DESTDIR)$(LIBDIR)
	$(INSTALL) -c -m 644 include/bsdiff/*.h $(DESTDIR)/$(INCLUDEDIR)/bsdiff
ifndef WITHOUT_MAN
	$(INSTALL_MAN) $(EXECUTABLES-y:=.1) $(DESTDIR)$(MAN1DIR)
endif
