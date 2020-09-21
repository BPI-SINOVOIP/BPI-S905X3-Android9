## Process this file with automake to produce Makefile.in

noinst_PROGRAMS = \
	network \
	rawout \
	wss

INCLUDES = -I$(top_srcdir)/src

LDADD = $(top_builddir)/src/libzvbi.la \
	$(PTHREAD_LIB) -lm $(PNG_LIB)
