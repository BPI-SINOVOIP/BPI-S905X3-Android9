
CWD	= "`pwd`"

dep: 	Makefile $(TOPDIR)/config.mk  $(SRCS)
	@$(RM) -f .depend 
	@for f in $(SRCS); do \
		g=`basename $$f | sed -e 's/\(.*\)\.\w/\1.o/'`; \
		$(CC) -M $(CPPFLAGS) -MQ $$g $$f | cat >> .depend ; \
	done


clean:
	@for dir in $(SRC_DIRS) ; do \
		$(MAKE) --no-print-directory -C $$dir clean ; \
	done
	@$(RM) $(LOCAL_OBJS)
	@$(RM) .build
	@$(RM) image/*
	@$(RM) $(LINT_OUT) $(LINT_DEF_FILE) $(LINT_SRC_FILE)*


soc_obj/%.o:	%.S
	@if [ ! -d soc_obj ]; then $(MKDIR) soc_obj; fi
ifdef SHOW_CC_OPT
	$(CC) $(AFLAGS) -c -o $@ $<
else
	@echo "Compiling $(notdir $@)" 1>&2
	@$(CC) $(AFLAGS) -c -o $@ $<
endif

soc_obj/%.o:	%.c
	@if [ ! -d soc_obj ]; then $(MKDIR) soc_obj; fi
ifdef SHOW_CC_OPT
	$(CC) $(CFLAGS) -c -o $@ $<
else
	@echo "Compiling $(notdir $@)" 1>&2
	@$(CC) $(CFLAGS) -c -o $@ $<
endif
	@echo $(CWD)/$< >> $(LINT_SRC_FILE).lnx

#@$(CYGPATH) $(CWD)/$< >> $(LINT_SRC_FILE)

LOCAL_OBJS = $(addprefix soc_obj/, $(subst .S,.o,$(LOCAL_ASMS)) $(subst .c,.o,$(LOCAL_SRCS)))

PHONY += $(SRC_DIRS)

$(SRC_DIRS):
#	@echo "Making $@ ..."
	@$(MAKE) --no-print-directory -C $@ _all


_all: $(sort $(SRC_DIRS)) $(sort $(LOCAL_OBJS))
	@for f in $(LOCAL_OBJS) ; do \
		echo -n "$(subst $(TOPDIR),.,$(CURDIR)/$$f)" >> $(TOPDIR)/.build ; \
		echo -n " " >> $(TOPDIR)/.build; \
	done

	
.PHONY: $(PHONY)
