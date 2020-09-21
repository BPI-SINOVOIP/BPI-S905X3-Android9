BASE=../../..

include $(BASE)/rule/def.mk

O_TARGET=netman aml 
aml_SRCS=aml.c
netman_SRCS=am_netman.c

include $(BASE)/rule/rule.mk

netman_EXT_OBJS=$(BUILDDIR)/../../am_mem/am_mem.o $(BUILDDIR)/../../am_misc/am_misc.o
all:
	@$(CC) -o $(BUILDDIR)/netman $(BUILDDIR)/am_netman.co $(netman_EXT_OBJS) -lpthread -lrt
