MODULES:=$(shell $(BASE)/rule/android_module.sh)
DEMODIR:=$(shell find . -name dvbdemotest | grep -v src)

PHONY+=all
all:
	TARGET_PRODUCT=$(TARGET_PRODUCT) TARGET_BUILD_VARIANT=$(TARGET_BUILD_VARIANT) ONE_SHOT_MAKEFILE=`pwd`/Android.mk make -C $(ANDROID_TOPDIR) all_modules
	if [ "x$(DEMODIR)" != "x" ]; then\
		make -C $(DEMODIR);\
	fi

PHONY+=clean
clean:dirclean

PHONY+=dirclean
CLEAN_MODULES:=$(patsubst %,clean-%,$(MODULES))
dirclean:
	@if [ "x$(CLEAN_MODULES)" != "x" ]; then\
		TARGET_PRODUCT=$(TARGET_PRODUCT) TARGET_BUILD_VARIANT=$(TARGET_BUILD_VARIANT) ONE_SHOT_MAKEFILE=`pwd`/Android.mk NO_PREBUILD=yes make -C $(ANDROID_TOPDIR) $(CLEAN_MODULES);\
	fi
	@if [ "x$(DEMODIR)" != "x" ]; then\
		make -C $(DEMODIR) dirclean;\
	fi

PHONY+=dirdistclean
dirdistclean:dirclean

