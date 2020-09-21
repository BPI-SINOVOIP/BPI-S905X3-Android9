BASE=..

include $(BASE)/rule/def.mk

MODULE_DEPEND=-L$(TARGET_BUILDDIR)/am_adp -lam_adp -L$(TARGET_BUILDDIR)/am_mw -lam_mw

SUBDIRS=am_tv

LIB_TARGET=am_app
am_app_OBJS=am_tv/am_tv

include $(BASE)/rule/rule.mk
