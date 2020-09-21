include $(ROOTDIR)/rule/rule_gen.mk

PHONY+=all

ifneq ($(o_target),)
O_TARGET:=$(patsubst %,$(BUILDDIR)/%.o,$(o_target))
OBJS:=$(patsubst %,$(BUILDDIR)/%.o,$($(o_target)_OBJS))
OBJS+=$(patsubst %.c,$(BUILDDIR)/%.co,$($(o_target)_SRCS))
EXT_OBJS:=$(patsubst %,%.o,$($(o_target)_EXT_OBJS))
all: $(O_TARGET)
else
ifneq ($(lib_target),)
LIB_TARGET:=$(patsubst %,$(BUILDDIR)/lib%.so,$(lib_target))
OBJS:=$(patsubst %,$(BUILDDIR)/%.o,$($(lib_target)_OBJS))
OBJS+=$(patsubst %.c,$(BUILDDIR)/%.co,$($(lib_target)_SRCS))
EXT_OBJS:=$(patsubst %,%.o,$($(lib_target)_EXT_OBJS))
all: $(LIB_TARGET)
else
ifneq ($(app_target),)
APP_TARGET:=$(patsubst %,$(BUILDDIR)/%,$(app_target))
OBJS:=$(patsubst %,$(BUILDDIR)/%.o,$($(app_target)_OBJS))
OBJS+=$(patsubst %.c,$(BUILDDIR)/%.co,$($(app_target)_SRCS))
EXT_OBJS:=$(patsubst %,%.o,$($(app_target)_EXT_OBJS))
LLIB_PATHES:=$(dir $($(app_target)_LIBS))
LLIBS:=$(notdir $($(app_target)_LIBS))
LDFLAGS:=$(patsubst %,-L$(BUILDDIR)/%,$(LLIB_PATHES)) $(patsubst %,-l%,$(LLIBS)) $(LDFLAGS)
LIBS:=$(join $(patsubst %,$(BUILDDIR)/%,$(LLIB_PATHES)), $(patsubst %,lib%.so,$(LLIBS)))
all: $(APP_TARGET) app_link

PHONY+=app_link
app_link:
	$(Q)BASE=$(BASE) APP=$(app_target) ROOTDIR=$(ROOTDIR) RELPATH=$(RELPATH) $(ROOTDIR)/rule/gen_app.sh > $(app_target)
	$(Q)chmod a+x $(app_target)

else
ifneq ($(java_lib),)

JAVA_DEX_LIBS:=$(patsubst %,$(BUILDDIR)/%.jar,$(java_lib))
JAVA_CLASS_LIBS:=$(patsubst %,$(BUILDDIR)/%.classes.jar,$(java_lib))
AIDL_JAVA_SRCS:=$(strip $(patsubst %.aidl,%.java,$(filter %.aidl,$($(java_lib)_SRCS))))
JAVA_SRCS:=$(filter %.java, $($(java_lib)_SRCS))
ALL_JAVA_SRCS:=$(JAVA_SRCS) $(AIDL_JAVA_SRCS)
EXT_OBJS:=$($(java_lib)_EXT_OBJS)

all: $(JAVA_DEX_LIBS)

else
all: builddir prebuild subdirs build_objs build_libs build_apps build_java_libs
endif
endif
endif
endif

PHONY+=config
config:
	$(Q) rm -rf $(ROOTDIR)/config/config.mk
	$(Q) cd $(ROOTDIR)/config; ln -s config_$(TARGET).mk config.mk

PHONY+=build_objs
build_objs:
	$(Q)for i in $(O_TARGET); do\
		make o_target=$$i;\
	done

PHONY+=build_java_libs
ifeq ($(TARGET),android)
build_java_libs:
	$(Q)for i in $(JAVA_LIBS); do\
		make java_lib=$$i;\
	done
else
build_java_libs:
endif

PHONY+=build_libs
build_libs:
	$(Q)for i in $(LIB_TARGET); do\
		make lib_target=$$i;\
	done

PHONY+=build_apps
build_apps:
	$(Q)for i in $(APP_TARGET); do\
		make app_target=$$i;\
	done

PHONY+=builddir
builddir:
	$(Q)$(MKDIR) $(BUILDDIR)
	$(Q)$(MKDIR) $(HOST_BUILDDIR)
ifneq ($(APIDIR),)
	$(Q)if [ x$(realpath $(ROOTDIR)/build/$(TARGET)/am_adp) != x$(realpath $(APIDIR)/build/$(TARGET)/am_adp) ]; then\
		rm -rf $(ROOTDIR)/build/$(TARGET)/am_adp;\
		ln -s $(APIDIR)/build/$(TARGET)/am_adp $(ROOTDIR)/build/$(TARGET)/am_adp;\
	fi
	$(Q)if [ x$(realpath $(ROOTDIR)/build/$(TARGET)/am_mw) != x$(realpath $(APIDIR)/build/$(TARGET)/am_mw) ]; then\
		rm -rf $(ROOTDIR)/build/$(TARGET)/am_mw;\
		ln -s $(APIDIR)/build/$(TARGET)/am_mw $(ROOTDIR)/build/$(TARGET)/am_mw;\
	fi
	$(Q)if [ x$(realpath $(ROOTDIR)/include) != x$(realpath $(APIDIR)/include) ]; then\
		rm -rf $(ROOTDIR)/include;\
		ln -s $(APIDIR)/include $(ROOTDIR)/include;\
	fi
endif

PHONY+=subdirs
subdirs:
	$(Q)for i in $(SUBDIRS); do \
		echo enter $$i...; \
		$(MAKE) -C $$i; \
		if [ $$? != 0 ]; then exit 1; fi; \
		echo leave $$i...; \
	done

PHONY+=prebuild
prebuild:
	$(Q)for i in $(PREBUILDDIRS); do \
		echo enter $$i...; \
		$(MAKE) -C $$i; \
		if [ $$? != 0 ]; then exit 1; fi; \
		echo leave $$i...; \
	done

PHONY+=dirclean

dirclean:
ifneq ($(java_lib),)
	$(Q)rm -rf $(JAVA_DEX_LIBS) $(JAVA_CLASS_LIBS) $(AIDL_JAVA_SRCS)
else
	$(Q)rm -rf $(APP_TARGET) *.jar
	$(Q)for i in $(SUBDIRS); do \
		$(MAKE) -C $$i dirclean; \
	done
	$(Q)for i in $(JAVA_LIBS); do \
		$(MAKE) dirclean java_lib=$$i;\
	done
endif

PHONY+=clean
clean: dirclean
	$(Q)$(INFO) clean...
	$(Q)rm -rf $(BUILDDIR)
	$(Q)rm -rf $(HOST_BUILDDIR)

DIRCLEAN_PATTERN=*.bak *~ .*.swp memwatch.log
PHONY+=dirdistclean
dirdistclean:
ifneq ($(java_lib),)
	$(Q)rm -rf $(AIDL_JAVA_SRCS)
else	
	$(Q)rm -rf $(DIRCLEAN_PATTERN)
	$(Q)for i in $(SUBDIRS); do \
                $(MAKE) -C $$i dirdistclean; \
        done
	$(Q)for i in $(JAVA_LIBS); do \
		$(MAKE) dirdistclean java_lib=$$i;\
	done
endif

PHONY+=distclean
distclean: dirdistclean
	$(Q)$(INFO) clean all...
	$(Q)rm -rf $(ROOTDIR)/build/* $(ROOTDIR)/config/user_info
	$(Q)rm -rf $(ROOTDIR)/doc/doxygen

PHONY+=add
add: $(FILE)

$(ROOTDIR)/config/user_info:
	$(Q)echo Plesse input your information
	$(Q)$(ROOTDIR)/rule/read_user_info.sh > $@

$(FILE): $(ROOTDIR)/config/user_info
	$(Q)$(INFO) CREATE $@...
	$(Q)ROOTDIR=$(ROOTDIR) TYPE=$(patsubst .%,%,$(suffix $@)) $(ROOTDIR)/rule/gen_file.sh > $@

PHONY+=mkdir
mkdir: $(DIR)

$(DIR):
	$(Q)$(INFO) MKDIR $@...
ifneq ($(dir $@),"./")
	$(Q)$(MAKE) DIR=$(patsubst %/,%,$(dir $@)) mkdir
endif
	$(Q)mkdir $@
	$(Q)ROOTDIR=$(ROOTDIR) BASE=$(BASE) DIR=$@ $(ROOTDIR)/rule/gen_makefile.sh > $@/Makefile

include $(ROOTDIR)/rule/rule_$(ARCH).mk

-include $(BUILDDIR)/*.d
-include $(HOST_BUILDDIR)/*.d

.PHONY:$(PHONY)
