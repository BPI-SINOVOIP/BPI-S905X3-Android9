ROOT_DIR = $(shell pwd)

export ROOT_DIR

all:
	make -C am_adp all
	make -C am_adp install
	make -C am_mw all
	make -C am_mw install
	make -C test all
install:
	make -C am_adp install
	make -C am_mw install
	make -C test install
#include "am_debug.h"
clean:
	make -C am_adp clean
	make -C am_mw clean
	make -C test clean
