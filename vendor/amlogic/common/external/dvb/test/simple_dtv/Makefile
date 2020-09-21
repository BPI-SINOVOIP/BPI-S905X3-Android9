BASE=../..

include $(BASE)/rule/def.mk

APP_TARGET=simple_dtv
simple_dtv_SRCS=dtv_main.c program.c window.c main_menu_win.c dvbc_param_win.c dvbt_param_win.c \
	restore_win.c prog_info_win.c prog_list_win.c scan_progress_win.c scan_result_win.c vol_win.c\
	gfx.c
simple_dtv_LIBS=../../am_mw/am_mw ../../am_adp/am_adp

include $(BASE)/rule/rule.mk
