include ./config.mk

##############################################################
# Global Variables
##############################################################
TOPDIR          := $(CURDIR)

export TOPDIR

ifeq ($(OS),Windows_NT)
TOPDIR_WIN := $(shell cygpath -m $(TOPDIR))
else
TOPDIR_WIN := $(TOPDIR)
endif

export TOPDIR_WIN

##############################################################
# Local Variables
##############################################################
IMAGE_DIR       := image
DOT_CONFIG      := .config
CONFIG_DIR      := defconfig
CONFIG_TARGET   := defconfig
MAKE_LOG        := make.log
LINT_DEF_FILE   := $(TOPDIR)/defs.lnt
LINT_SRC_FILE   := $(TOPDIR)/all_src.lnt
LINT_OUT        := lint_out.txt
STRIP_SH        := $(TOPDIR)/scripts/strip_dmsg.sh
TARGET_DIR      := ../../image /lib/firmware

export LINT_SRC_FILE

conf_exist      := $(shell [ ! -f "$(DOT_CONFIG)" ] || echo "y" )

ifeq ($(TARGET),)
ifeq ($(conf_exist),y)
TARGET          := $(shell cat $(DOT_CONFIG) | \
                     grep -e '^CONFIG_TARGET[[:blank:]]*=[[:blank:]]*' | \
                     sed 's/=/ /g' | awk '{ print $$2 }')
HCI_TYPE	:= $(shell cat $(DOT_CONFIG) | \
		     grep -e '^CONFIG_HCI[[:blank:]]*=[[:blank:]]*' | \
		     sed 's/=/ /g' | awk '{ print $$2 }')
IMAGE_NAME	:= $(addprefix $(IMAGE_DIR)/$(TARGET)-, $(HCI_TYPE))
endif
endif


CONFIG_FILES    := $(addprefix $(CONFIG_DIR)/, \
                   $(subst $(CONFIG_TARGET),,$(shell ls -1 $(TOPDIR)/$(CONFIG_DIR))))
TARGET_LIST     := $(shell cat $(CONFIG_FILES) | \
                     grep -e '^CONFIG_TARGET[[:blank:]]*=[[:blank:]]*' | \
                     sed 's/=/ /g' | awk '{ print $$2 }')


##############################################################
# Source directories to Make
##############################################################
SRC_DIRS        := bsp
SRC_DIRS        += apps
SRC_DIRS        += rtos
SRC_DIRS        += driver
SRC_DIRS        += lib
SRC_DIRS	      += cli



.PHONY: all config target_list target_check

all: target_check $(if $(TARGET), $(sort $(SRC_DIRS)), )
	@if [ "$(TARGET)" = "" ] ; then \
	    echo "No Target is specified !!"; \
	    exit 0; \
	fi
	@echo "Try linking ...\n" $(shell cat .build)
	@$(CC) $(CFLAGS) $(LDFLAGS) $(shell cat .build)
	@$(OBJCOPY) -O binary $(IMAGE_NAME).elf $(IMAGE_NAME).bin
	@$(OBJDUMP) -D -S $(IMAGE_NAME).elf > $(IMAGE_NAME).asm
	@echo "Done.\n"
#	#@${BIN2MIF} $(IMAGE_NAME).bin $(IMAGE_NAME).mif 32
#	@cd $(TOPDIR) && $(STRIP_SH)
	
scratch: .config
	@$(RM) $(MAKE_LOG)
	@echo "Cleaning ..."
	@$(MAKE) clean
	@echo "Building ..."
	@echo "CFLAGS = $(CFLAGS)"
	@echo $(INCLUDE) $(GLOBAL_DEF) > defs.lnt
	@$(MAKE) all 2>&1 | $(LOG_CC) $(MAKE_LOG)
	@$(MAKE) install

.config: config

config:
	@if [ "$(TARGET)" != "" ] ; then \
	    for f in $(CONFIG_FILES) ; do \
		if [ "$$f" = "$(CONFIG_DIR)/$(TARGET)" ] ; then \
		    echo "copy $$f to $(DOT_CONFIG) ..."; \
		    $(CP) $$f $(DOT_CONFIG); \
		    $(CP) $$f defconfig/defconfig; \
		fi; \
	    done ; \
	elif [ -f $(CONFIG_DIR)/$(CONFIG_TARGET) ] ; then \
	    $(CP) $(CONFIG_DIR)/$(CONFIG_TARGET) $(DOT_CONFIG); \
	    echo "Copy $(CONFIG_DIR)/$(CONFIG_TARGET) to $(DOT_CONFIG) ..."; \
	else \
	    echo "Usage: make config TARGET=<TARGET_NAME)"; \
	    echo "       OR"; \
	    echo "       copy your target config file to $(CONFIG_DIR)/$(CONFIG_TARGET)"; \
	fi


target_list:
	@$(TOPDIR)/scripts/show-targets.sh $(TOPDIR)/defconfig/

target_check:
	@if [ "$(TARGET)" = "" ]; then \
	    echo "Please 'make config' first!!"; \
	    exit 0; \
	fi
	@$(RM) -rf .build
	@echo "make target '$(TARGET)'"

mrproper: clean
	@$(RM) .config
	@$(RM) $(MAKE_LOG) 

install:
	@for dir in $(TARGET_DIR); do \
	    if [ "$$dir" != "" -a -d "$$dir" ]; then \
	        echo "Copy $(IMAGE_NAME).bin to $$dir"; \
	        cp $(IMAGE_NAME).bin $$dir; \
	    fi; \
	done

lint:
	@$(RM) $(LINT_OUT) $(LINT_SRC_FILE)
	@cygpath_exec=`echo $(CYGPATH) | cut -d ' ' -f1`; which $$cygpath_exec > /dev/null || (echo No $$cygpath_exec; exit 1;)
	@for lnx_src in `cat $(LINT_SRC_FILE).lnx`; do\
	    $(CYGPATH) $$lnx_src >> $(LINT_SRC_FILE); \
	 done
	@echo $(GLOBAL_DEF) >  $(LINT_DEF_FILE)
	@for inc in $(subst -I,,$(INCLUDE)); do \
	    echo -n '-I' >> $(LINT_DEF_FILE); \
	    $(CYGPATH) $$inc >> $(LINT_DEF_FILE); \
	done
ifdef SHOW_LINT
	$(LINT) $(LINT_OPT)
else
	$(LINT) $(LINT_OPT) -os"($(LINT_OUT))"
endif

##############################################################
# Make rules
##############################################################
include ./rules.mk

