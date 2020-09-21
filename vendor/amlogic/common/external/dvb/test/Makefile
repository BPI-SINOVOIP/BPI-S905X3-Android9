LOCAL_PATH = $(ROOT_DIR)/test

SUB_DIRS = am_fend_test am_smc_test am_dsc_test am_dvr_test am_ci_test am_userdata_test am_epg_test am_av_test am_dmx_test am_scan_test

all :
	@for dir in $(SUB_DIRS); do \
		echo "$$dir";\
		make -C $$dir all;\
		if [ "$$?" != "0" ]; then\
			exit 1;\
		fi;\
	done

install:
	@for dir in $(SUB_DIRS); do \
		echo "$$dir";\
		make -C $$dir install;\
	done

clean:
	@for dir in $(SUB_DIRS); do \
		echo "$$dir";\
		make -C $$dir clean;\
	done
