$(BUILDDIR)/%.co:%.c
	$(Q)$(INFO) CC $< \> $(patsubst $(BUILDDIR)/%,%,$@)...
	$(Q)$(CC) $(CFLAGS) $(PIC) -MMD $($@:.co=.d) -c -o $@ $<

$(BUILDDIR)/classes/%.class:%.java
	$(Q)$(INFO) JAVAC $< \> $(patsubst $(BUILDDIR)/classes/%,%,$@)...
	$(Q)$(JAVAC) $(JAVACFLAGS) -d $(BUILDDIR)/classes $<

$(AIDL_JAVA_SRCS):%.java:%.aidl
	$(Q)$(INFO) AIDL $< \> $@...
	$(Q)$(AIDL) -d$(BUILDDIR)/classes/$(@:.java=.P) -b -I. -Isrc $(AIDLFLAGS) $< $@

ifneq "$(AIDL_JAVA_SRCS)" ""
-include $(addprefix $(BUILDDIR)/classes/,$(AIDL_JAVA_SRCS:%.java=%.P))
endif


ifneq ($(o_target),)
$(O_TARGET): $(OBJS) $(EXT_OBJS)
	$(Q)$(INFO) LD $(patsubst $(BUILDDIR)/%,%,$(OBJS)) $(EXT_OBJS) \> $(patsubst $(BUILDDIR)/%,%,$@)...
	$(Q)$(LD) -r -o $@ $^
endif


ifneq ($(java_lib),)
# For a list of jar files, unzip them to a specified directory,
# but make sure that no META-INF files come along for the ride.
#
# $(1): files to unzip
# $(2): destination directory
define unzip-jar-files
  $(hide) for f in $(1); \
  do \
    if [ ! -f $$f ]; then \
      echo Missing file $$f; \
      exit 1; \
    fi; \
    unzip -qo $$f -d $(2); \
    (cd $(2) && rm -rf META-INF); \
  done
endef

# emit-line, <word list>, <output file>
define emit-line
   $(if $(1),echo -n '$(strip $(1)) ' >> $(2))
endef

# dump-words-to-file, <word list>, <output file>
define dump-words-to-file
        @rm -f $(2)
        @$(call emit-line,$(wordlist 1,200,$(1)),$(2))
        @$(call emit-line,$(wordlist 201,400,$(1)),$(2))
        @$(call emit-line,$(wordlist 401,600,$(1)),$(2))
        @$(call emit-line,$(wordlist 601,800,$(1)),$(2))
        @$(call emit-line,$(wordlist 801,1000,$(1)),$(2))
        @$(call emit-line,$(wordlist 1001,1200,$(1)),$(2))
        @$(call emit-line,$(wordlist 1201,1400,$(1)),$(2))
        @$(call emit-line,$(wordlist 1401,1600,$(1)),$(2))
        @$(call emit-line,$(wordlist 1601,1800,$(1)),$(2))
        @$(call emit-line,$(wordlist 1801,2000,$(1)),$(2))
        @$(call emit-line,$(wordlist 2001,2200,$(1)),$(2))
        @$(call emit-line,$(wordlist 2201,2400,$(1)),$(2))
        @$(call emit-line,$(wordlist 2401,2600,$(1)),$(2))
        @$(call emit-line,$(wordlist 2601,2800,$(1)),$(2))
        @$(call emit-line,$(wordlist 2801,3000,$(1)),$(2))
        @$(call emit-line,$(wordlist 3001,3200,$(1)),$(2))
        @$(call emit-line,$(wordlist 3201,3400,$(1)),$(2))
        @$(call emit-line,$(wordlist 3401,3600,$(1)),$(2))
        @$(call emit-line,$(wordlist 3601,3800,$(1)),$(2))
        @$(call emit-line,$(wordlist 3801,4000,$(1)),$(2))
        @$(if $(wordlist 4001,4002,$(1)),$(error Too many words ($(words $(1)))))
endef

JAVA_DEX:=$(patsubst %,$(BUILDDIR)/classes.dex,$(java_lib))

$(JAVA_DEX_LIBS):$(JAVA_CLASS_LIBS)
	$(Q)$(INFO) DX $(patsubst $(BUILDDIR)/%,%,$(JAVA_CLASS_LIBS)) \> $(patsubst $(BUILDDIR)/%,%,$@)...
	$(Q)$(DX) --dex --output=$(JAVA_DEX) $<
	$(Q)$(INFO) JAR $(notdir $(JAVA_DEX)) \> $(notdir $@)...
	$(Q)$(JAR) cvf $@ -C $(BUILDDIR) $(notdir $(JAVA_DEX))
	$(Q)rm -f $(JAVA_DEX)

$(JAVA_CLASS_LIBS):$(ALL_JAVA_SRCS) $(EXT_OBJS)
	$(Q)$(INFO) JAVAC $(notdir $(ALL_JAVA_SRCS)) \> $(patsubst %.java,%.class,$(notdir $(ALL_JAVA_SRCS)))...
	$(call dump-words-to-file,$(ALL_JAVA_SRCS),$(BUILDDIR)/java-source-list)
	$(Q) tr ' ' '\n' < $(BUILDDIR)/java-source-list \
                  | sort -u > $(BUILDDIR)/java-source-list-uniq
	$(Q)rm -rf $(BUILDDIR)/classes
	$(Q)$(MKDIR) $(BUILDDIR)/classes
	$(Q)$(JAVAC) $(JAVACFLAGS) -d $(BUILDDIR)/classes \
                  \@$(BUILDDIR)/java-source-list-uniq
	$(Q)$(INFO) JAR $(patsubst %.java,%.class,$(notdir $(ALL_JAVA_SRCS))) $(EXT_OBJS) \> $(notdir $@)...
	$(call unzip-jar-files,$(EXT_OBJS), $(BUILDDIR)/classes)
	$(Q)$ rm -rf $(addprefix $(BUILDDIR)/classes/,$(AIDL_JAVA_SRCS:%.java=%.P))
	$(Q)$(JAR) cvf $@ -C $(BUILDDIR)/classes/ .	
	$(Q)rm -rf $(BUILDDIR)/classes
	$(Q)rm -f $(BUILDDIR)/java-source-list-uniq
	$(Q)rm -f $(BUILDDIR)/java-source-list
	
endif

ifeq ($(TARGET),android)

ifneq ($(lib_target),)
$(LIB_TARGET):$(OBJS) $(EXT_OBJS)
	$(Q)$(INFO) LD $(patsubst $(BUILDDIR)/%,%,$(OBJS)) $(EXT_OBJS) \> $(patsubst $(BUILDDIR)/%,%,$@)...
	$(Q)$(CC)    $(SHARELIB_LDFLAGS) -o $@  $^  $(MODULE_DEPEND) $(LDFLAGS)
	$(Q)if [ x$(lib_target) = xam_adp ]; then cp $@ $(ANDROID_TOPDIR)/out/target/product/$(TARGET_PRODUCT)/obj/lib; cp $@ $(ANDROID_TOPDIR)/out/target/product/$(TARGET_PRODUCT)/system/lib; fi
	$(Q)if [ x$(lib_target) = xam_mw ]; then cp $@ $(ANDROID_TOPDIR)/out/target/product/$(TARGET_PRODUCT)/obj/lib; cp $@ $(ANDROID_TOPDIR)/out/target/product/$(TARGET_PRODUCT)/system/lib; fi
endif
else
ifneq ($(lib_target),)
$(LIB_TARGET):$(OBJS) $(EXT_OBJS)
	$(Q)$(INFO) AR $(patsubst $(BUILDDIR)/%,%,$(OBJS)) $(EXT_OBJS) \> $(patsubst $(BUILDDIR)/%,%,$@)...
	$(Q)$(AR) -r  -o $@ $^ 
	$(Q)$(INFO) RANLIB $(patsubst $(BUILDDIR)/%,%,$@)...
	$(Q)$(RANLIB) $@
endif

endif


ifneq ($(app_target),)
$(APP_TARGET):$(OBJS) $(EXT_OBJS) $(LIBS)
	$(Q)$(INFO) CC $(patsubst $(BUILDDIR)/%,%,$(OBJS)) $(EXT_OBJS) \> $(patsubst $(BUILDDIR)/%,%,$@)...
ifeq ($(TARGET),android)

#	$(Q)$(CC) -o $@ $(OBJS) $(EXT_OBJS) /home/stb/work/am_stb_api/android/ndk/lib/crtbegin_dynamic.o /home/stb/work/am_stb_api/android/ndk/lib/crtend_android.o -nostdlib -Bdynamic -Wl,-T,/home/stb/work/am_stb_api/android/armelf.x -Wl,-dynamic-linker,/system/bin/linker -L/home/stb/work/am_stb_api/android/ndk/lib -lc -lgcc -lm -ldl -lstdc++
	$(Q)$(CC) -nostdlib -o  $@ $(OBJS) $(EXT_OBJS)    $(CRT_BEGIN_O) $(CRT_END_O) $(LDFLAGS) 
else
	$(Q)$(CC) -o $@ $(OBJS) $(EXT_OBJS)   $(LDFLAGS) 

endif
endif
