/*
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * Description: 
*/
#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif

#define AM_DEBUG_LEVEL 1
#define _GNU_SOURCE

#include <assert.h>

#include "am_types.h"
#include "am_debug.h"

#include "libdvben50221/en50221_transport.h"
#include "libdvben50221/en50221_session.h"
#include "libdvben50221/en50221_app_mmi.h"
#include "libdvben50221/en50221_stdcam.h"

#include "am_ci.h"
#include "am_ci_internal.h"
#include <am_thread.h>

#define AM_CI_DEV_COUNT 2

#define LOCAL_SAVE_STAT 0


typedef struct _slot_info_s _slot_info_t;


struct _slot_info_s{
	/*ai info*/
	struct {
		int enable;
		uint8_t application_type;
		uint16_t application_manufacturer;
		uint16_t manufacturer_code;
		uint8_t menu_string_length;
		uint8_t *menu_string;
	}ai_info;

	/*ca info*/
	struct {
		int enable;
		int ca_id_count;
		unsigned short *ca_ids;
	}ca_info;

#if LOCAL_SAVE_STAT
	/*tmp*/
	struct {
		int enable;
		uint16_t session_number;
		uint8_t blind_answer;
		uint8_t expected_answer_length;
		uint8_t *text;
		uint32_t text_size;
	}enq;

	struct {
		int enable;
		uint16_t session_number;
		app_mmi_text_t title,
		app_mmi_text_t sub_title;
		app_mmi_text_t bottom;
		uint32_t item_count;
		app_mmi_text_t *items;
		uint32_t item_raw_length;
		uint8_t *items_raw;
	}menulist[2];
	enum {
		slot_info_menulist_idx_menu,
		slot_info_menulist_idx_list,
		slot_info_menulist_idx_max,
	}
#endif

} ;

struct _ci_s{
	int used;

	int used_by_caman;

	int dev_id;

	int slot_no;

	int status;
#define ci_status_closed 0
#define ci_status_open 1
#define ci_status_started 2
#define ci_status_all 0xffff

	struct en50221_transport_layer *tl;
	struct en50221_session_layer *sl;
	struct en50221_stdcam *stdcam;

	int ca_resource_connected;

	_slot_info_t *slot_info;

	ai_callback ai_cb;
	void *ai_cb_arg;
	ca_info_callback ca_info_cb;
	void *ca_info_cb_arg;
	mmi_close_callback mmi_close_cb;
	void *mmi_close_cb_arg;
	mmi_display_control_callback mmi_display_control_cb;
	void *mmi_display_control_cb_arg;
	mmi_enq_callback mmi_enq_cb;
	void *mmi_enq_cb_arg;
	mmi_menu_callback mmi_menu_cb;
	void *mmi_menu_cb_arg;
	mmi_menu_callback mmi_list_cb;
	void *mmi_list_cb_arg;

	pthread_t	               camthread;
	int                          camthread_quit;

	pthread_mutex_t     lock;

	//pthread_t	               workthread;
	//int                          workthread_quit;
	//pthread_cond_t       cond;
};

struct {
	pthread_mutex_t     lock;
	_ci_t cis[AM_CI_DEV_COUNT];
} gci =
{

#ifdef PTHREAD_RECURSIVE_MUTEX_INITIALIZER
	.lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER,
	.cis =
		{
			[0]={.lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER,},
			[1]={.lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER,},
		},
#else
	.lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP,
	.cis =
		{
			[0]={.lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP,},
			[1]={.lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP,},
		},
#endif
};

static inline AM_ErrorCode_t get_ci(AM_CI_Handle_t handle, _ci_t **pci)
{
	_ci_t *ci = (_ci_t *)handle;
	int i;

	assert(handle);
	assert(pci);

	pthread_mutex_lock(&gci.lock);

	if (ci->used)
	{
		for (i=0; i<AM_CI_DEV_COUNT; i++)
		{
			if (&gci.cis[i] == ci) {
				*pci = ci;
				pthread_mutex_unlock(&gci.lock);
				return AM_SUCCESS;
			}
		}
	}
	*pci = NULL;
	pthread_mutex_unlock(&gci.lock);
	return AM_CI_ERROR_BAD_PARAM;;
}

static inline AM_ErrorCode_t get_ci_new(int dev_no, int slot, _ci_t **pci)
{
	AM_ErrorCode_t err;
	_ci_t *ci, *ci_tmp=NULL;
	int i;

	assert(pci);

	pthread_mutex_lock(&gci.lock);

	for (i=0; i<AM_CI_DEV_COUNT; i++)
	{
		if (!gci.cis[i].used && !ci_tmp)
			ci_tmp= &gci.cis[i];

		if (gci.cis[i].used
			&& (gci.cis[i].dev_id == dev_no)
			&& (gci.cis[i].slot_no == slot))
		{
			*pci = ci;
			pthread_mutex_unlock(&gci.lock);
			return AM_SUCCESS;
		}
	}

	if (ci_tmp)
	{
		*pci = ci_tmp;
	}
	else
	{
		*pci = NULL;
		pthread_mutex_unlock(&gci.lock);
		return AM_CI_ERROR_MAX_DEV;
	}

	pthread_mutex_unlock(&gci.lock);
	return AM_SUCCESS;
}

#if LOCAL_SAVE_STAT
static inline void rm_mmi_text(app_mmi_text_t *t)
{
	if (t->text) {
		free(t->text);
		t->text = NULL;
	}
}
#endif

static int ci_free_slots_info(_ci_t *ci)
{
	_slot_info_t *slot;

	slot = ci->slot_info;
	if (slot->ai_info.menu_string)
		free(slot->ai_info.menu_string);
	slot->ai_info.menu_string = NULL;
	slot->ai_info.enable = 0;
	if (slot->ca_info.ca_ids)
		free(slot->ca_info.ca_ids);
	slot->ca_info.ca_ids = NULL;
	slot->ca_info.enable = 0;

#if LOCAL_SAVE_STAT

	if (slot->enq.text)
	{
		free(slot->enq.text);
		slot->enq.text = NULL;
	}

	rm_mmi_text(&slot->menulist[0].title);
	rm_mmi_text(&slot->menulist[0].sub_title);
	rm_mmi_text(&slot->menulist[0].bottom);
	if (slot->menulist[0].items)
		free(slot->menulist[0].items);
	slot->menulist[0].items = NULL;
	if (slot->menulist[0].items_raw)
		free(slot->menulist[0].items_raw);
	slot->menulist[0].items_raw = NULL;

	rm_mmi_text(&slot->menulist[1].title);
	rm_mmi_text(&slot->menulist[1].sub_title);
	rm_mmi_text(&slot->menulist[1].bottom);
	if (slot->menulist[1].items)
		free(slot->menulist[1].items);
	slot->menulist[1].items = NULL;
	if (slot->menulist[1].items_raw)
		free(slot->menulist[1].items_raw);
	slot->menulist[1].items_raw = NULL;
#endif
	free(ci->slot_info);
	return 0;
}


static int ci_ai_callback(void *arg, uint8_t slot_id, uint16_t session_number,
			     uint8_t application_type, uint16_t application_manufacturer,
			     uint16_t manufacturer_code, uint8_t menu_string_length,
			     uint8_t *menu_string)
{
	_ci_t *ci = (_ci_t*) arg;
	(void) slot_id;
	(void) session_number;

	AM_DEBUG(1, "CAM Application type: %02x\n", application_type);
	AM_DEBUG(1, "CAM Application manufacturer: %04x\n", application_manufacturer);
	AM_DEBUG(1, "CAM Manufacturer code: %04x\n", manufacturer_code);
	AM_DEBUG(1, "CAM Menu string: %.*s\n", menu_string_length, menu_string);


	pthread_mutex_lock(&ci->lock);

	_slot_info_t *slot;
	slot = ci->slot_info;

	slot->ai_info.application_type = application_type;
	slot->ai_info.application_manufacturer = application_manufacturer;
	slot->ai_info.manufacturer_code = manufacturer_code;
	slot->ai_info.menu_string_length = menu_string_length;
	if (slot->ai_info.menu_string)
		free(slot->ai_info.menu_string);
	if (slot->ai_info.menu_string_length)
	{
		slot->ai_info.menu_string = malloc(slot->ai_info.menu_string_length);
		assert(slot->ai_info.menu_string);
		memcpy(slot->ai_info.menu_string, menu_string, menu_string_length);
	}
	slot->ai_info.enable = 1;

	if (ci->ai_cb)
		ci->ai_cb(ci->ai_cb_arg, slot_id, session_number,
				application_type, application_manufacturer,
				manufacturer_code, menu_string_length, menu_string);

	pthread_mutex_unlock(&ci->lock);

	return 0;
}

static int ci_ca_pmt_relay_callback(void *arg, uint8_t slot_id, uint16_t session_number,
							struct en50221_app_pmt_reply *reply,
						  uint32_t reply_size)
{
		if (reply == NULL) {
			AM_DEBUG(1, "pmt relay pro:---");
		} else {
				AM_DEBUG(1, "pmt relay pro:%d enable= %d ca flag= %d\r\n",
						reply->program_number,
						reply->CA_enable_flag,
						reply->CA_enable);
		}
		return 0;
}

static int ci_ca_info_callback(void *arg, uint8_t slot_id, uint16_t session_number, uint32_t ca_id_count, uint16_t *ca_ids)
{
	_ci_t *ci = (_ci_t*) arg;
	(void) slot_id;
	(void) session_number;

	AM_DEBUG(1, "CAM supports the following ca system ids:\n");
	uint32_t i;
	for (i=0; i< ca_id_count; i++) {
		AM_DEBUG(1, "  0x%04x\n", ca_ids[i]);
	}

	pthread_mutex_lock(&ci->lock);

	_slot_info_t *slot;
	slot = ci->slot_info;
	slot->ca_info.ca_id_count = ca_id_count;
	if (slot->ca_info.ca_ids)
		free(slot->ca_info.ca_ids);
	if (ca_id_count)
	{
		slot->ca_info.ca_ids = calloc(sizeof(unsigned short), ca_id_count);
		assert(slot->ca_info.ca_ids);
		memcpy(slot->ca_info.ca_ids, ca_ids, ca_id_count*2);
	}
	slot->ca_info.enable = 1;

	if (ci->ca_info_cb)
		ci->ca_info_cb(ci->ca_info_cb_arg, slot_id, session_number, ca_id_count, ca_ids);

	pthread_mutex_unlock(&ci->lock);

	ci->ca_resource_connected = 1;
	return 0;
}

static int ci_mmi_close_callback(void *arg, uint8_t slot_id, uint16_t session_number,
				    uint8_t cmd_id, uint8_t delay)
{
	_ci_t *ci = (_ci_t*) arg;
	(void) slot_id;
	(void) session_number;
	(void) cmd_id;
	(void) delay;

	pthread_mutex_lock(&ci->lock);

#if LOCAL_SAVE_STAT
	_slot_info_t *slot = ci->slot_info;

	if ((session_number == slot->enq.session_number)
		&& (slot->enq.enable) )
	{
		if (slot->enq.text)
		{
			free(slot->enq.text);
			slot->enq.text = NULL;
		}

		slot->enq.enable = 0;
	}

	int idx, i;
	for (idx = 0; idx<slot_info_menulist_idx_max; idx++)
	{
		if ((slot->menulist[idx].enable)
			&& (session_number == slot->menulist[idx].session_number) )
		{
			rm_mmi_text(&slot->menulist[idx].title);
			rm_mmi_text(&slot->menulist[idx].sub_title);
			rm_mmi_text(&slot->menulist[idx].bottom);
			if (slot->menulist[idx].items)
				free(slot->menulist[idx].items);
			slot->menulist[idx].items = NULL;
			if (slot->menulist[idx].items_raw)
				free(slot->menulist[idx].items_raw);
			slot->menulist[idx].items_raw = NULL;

			slot->menulist[idx].enable = 0;
		}
	}
#endif

	if (ci->mmi_close_cb)
		ci->mmi_close_cb(ci->mmi_close_cb_arg, slot_id, session_number, cmd_id, delay);

	pthread_mutex_unlock(&ci->lock);

	// note: not entirely correct as its supposed to delay if asked
	//mmi_state = MMI_STATE_CLOSED;
	return 0;
}

static int ci_mmi_display_control_callback(void *arg, uint8_t slot_id, uint16_t session_number,
					      uint8_t cmd_id, uint8_t mmi_mode)
{
	struct en50221_app_mmi_display_reply_details reply;
	_ci_t *ci = (_ci_t*) arg;
	(void) slot_id;

	// don't support any commands but set mode
	if (cmd_id != MMI_DISPLAY_CONTROL_CMD_ID_SET_MMI_MODE) {
		en50221_app_mmi_display_reply(ci->stdcam->mmi_resource, session_number,
					      MMI_DISPLAY_REPLY_ID_UNKNOWN_CMD_ID, &reply);
		return 0;
	}

	// we only support high level mode
	if (mmi_mode != MMI_MODE_HIGH_LEVEL) {
		en50221_app_mmi_display_reply(ci->stdcam->mmi_resource, session_number,
					      MMI_DISPLAY_REPLY_ID_UNKNOWN_MMI_MODE, &reply);
		return 0;
	}

	// ack the high level open
	reply.u.mode_ack.mmi_mode = mmi_mode;
	en50221_app_mmi_display_reply(ci->stdcam->mmi_resource, session_number,
				      MMI_DISPLAY_REPLY_ID_MMI_MODE_ACK, &reply);

	pthread_mutex_lock(&ci->lock);

	if (ci->mmi_display_control_cb)
		ci->mmi_display_control_cb(ci->mmi_display_control_cb_arg, slot_id, session_number,
					      cmd_id, mmi_mode);

	pthread_mutex_unlock(&ci->lock);

	//mmi_state = MMI_STATE_OPEN;
	return 0;
}

static int ci_mmi_enq_callback(void *arg, uint8_t slot_id, uint16_t session_number,
				  uint8_t blind_answer, uint8_t expected_answer_length,
				  uint8_t *text, uint32_t text_size)
{
	_ci_t *ci = (_ci_t*) arg;
	(void) slot_id;
	(void) session_number;


	AM_DEBUG(1, "%.*s: ", text_size, text);

	pthread_mutex_lock(&ci->lock);

	if (ci->mmi_enq_cb)
		ci->mmi_enq_cb(ci->mmi_enq_cb_arg, slot_id, session_number,
					      blind_answer, expected_answer_length,
						text, text_size);
#if LOCAL_SAVE_STAT
	else
	{
		/*save this to callback later*/
		_slot_info_t *slot = ci->slot_info;
		slot->enq.session_number = session_number;
		slot->enq.blind_answer = blind_answer;
		slot->enq.expected_answer_length = expected_answer_length;
		slot->enq.text_size = text_size;
		if (slot->enq.text)
			free(slot->enq.text);
		if (text_size)
		{

			slot->enq.text = malloc(text_size);
			if (slot->enq.text)
				memcpy(slot->enq.text, text, text_size);
		}
		slot->enq.enable = 1;
	}
#endif

	pthread_mutex_unlock(&ci->lock);

	//mmi_enq_blind = blind_answer;
	//mmi_enq_length = expected_answer_length;
	//mmi_state = MMI_STATE_ENQ;
	return 0;
}

static int mmi_menu_list_callback(void *arg, uint8_t slot_id, uint16_t session_number,
				   struct en50221_app_mmi_text *title,
				   struct en50221_app_mmi_text *sub_title,
				   struct en50221_app_mmi_text *bottom,
				   uint32_t item_count, struct en50221_app_mmi_text *items,
				   uint32_t item_raw_length, uint8_t *items_raw, int menu_not_list)
{
	_ci_t *ci = (_ci_t*) arg;
	(void) slot_id;
	(void) session_number;
	(void) item_raw_length;
	(void) items_raw;

	AM_DEBUG(1, "--[%s]----------------------------\n", menu_not_list? "menu" : "list");

	if (title->text_length) {
		AM_DEBUG(1, "%.*s\n", title->text_length, title->text);
	}
	if (sub_title->text_length) {
		AM_DEBUG(1, "%.*s\n", sub_title->text_length, sub_title->text);
	}

	uint32_t i;
	AM_DEBUG(1, "0. Quit menu\n");
	for (i=0; i< item_count; i++) {
		AM_DEBUG(1, "%i. %.*s\n", i+1, items[i].text_length, items[i].text);
	}

	if (bottom->text_length) {
		AM_DEBUG(1, "%.*s\n", bottom->text_length, bottom->text);
	}
	AM_DEBUG(1, "Enter option: ");


	pthread_mutex_lock(&ci->lock);

	if (menu_not_list && ci->mmi_menu_cb)
	{
		ci->mmi_menu_cb(ci->mmi_menu_cb_arg, slot_id, session_number,
						(app_mmi_text_t*)title,
						(app_mmi_text_t*)sub_title,
						(app_mmi_text_t*)bottom,
						item_count,
						(app_mmi_text_t*)items,
						item_raw_length, items_raw);
	}
	else if(!menu_not_list && ci->mmi_list_cb)
	{
		ci->mmi_list_cb(ci->mmi_list_cb_arg, slot_id, session_number,
						(app_mmi_text_t*)title,
						(app_mmi_text_t*)sub_title,
						(app_mmi_text_t*)bottom,
						item_count,
						(app_mmi_text_t*)items,
						item_raw_length, items_raw);
	}
#if LOCAL_SAVE_STAT
	else
	{
		/*save this to callback later*/

		_slot_info_t *slot = ci->slot_info;
		int idx = menu_not_list ? slot_info_menulist_idx_menu : slot_info_menulist_idx_list;

		if (slot->menulist[idx].enable)
		{
			AM_DEBUG(1, "CI: WARING: REQUEST from CAM lost. More than one %s requsted, the previous one will be dropped.",
				menu_not_list? "menu" : "list");
		}

		slot->menulist[idx].session_number = session_number;

		#define cp_mmi_text(to, from) \
			do { \
				if ((to)->text) \
					free((to)->text);	\
				(to)->text_length = (from)->text_length; \
				if ((from)->text_length) \
					if ((to)->text = malloc((from)->text_length)) \
						assert((to)->text);\
						memcpy((to)->text, (from)->text, (from)->text_length); \
			} while(0)

		cp_mmi_text(&slot->menulist[idx].title, title);
		cp_mmi_text(&slot->menulist[idx].sub_title, sub_title);
		cp_mmi_text(&slot->menulist[idx].bottom, bottom);
		cp_mmi_text(&slot->menulist[idx].title, title);

		/*cp items*/
		slot->menulist[idx].item_count = item_count;
		if (slot->menulist[idx].items)
			free(slot->menulist[idx].items);
		if (item_count)
		{
			int i, total=0;
			for (i=0; i<item_count; i++)
				total += items[i].text_length+4;
			if (total)
				slot->menulist[idx].items = malloc(total);
			assert(slot->menulist[idx].items);
			for (i=0; i<item_count; i++)
				cp_mmi_text(&slot->menulist[idx].items[i], items[i]);
		}

		/*cp items_raw*/
		slot->menulist[idx].item_raw_length = item_raw_length;
		if (slot->menulist[idx].items_raw)
			free(slot->menulist[idx].items_raw);
		if (item_raw_length)
			slot->menulist[idx].items_raw = malloc(item_raw_length);
		assert(slot->menulist[idx].items_raw);
		memcpy(slot->menulist[idx].items_raw, items_raw);

		/*enable this*/
		slot->menulist[idx].enable = 1;
	}
#endif

	pthread_mutex_unlock(&ci->lock);

	//mmi_state = MMI_STATE_MENU;
	return 0;
}

static int ci_mmi_menu_callback(void *arg, uint8_t slot_id, uint16_t session_number,
				   struct en50221_app_mmi_text *title,
				   struct en50221_app_mmi_text *sub_title,
				   struct en50221_app_mmi_text *bottom,
				   uint32_t item_count, struct en50221_app_mmi_text *items,
				   uint32_t item_raw_length, uint8_t *items_raw)
{
	return mmi_menu_list_callback(arg, slot_id, session_number,
								   title,sub_title,bottom,
								   item_count, items,
								   item_raw_length, items_raw, 1);
}

static int ci_mmi_list_callback(void *arg, uint8_t slot_id, uint16_t session_number,
				   struct en50221_app_mmi_text *title,
				   struct en50221_app_mmi_text *sub_title,
				   struct en50221_app_mmi_text *bottom,
				   uint32_t item_count, struct en50221_app_mmi_text *items,
				   uint32_t item_raw_length, uint8_t *items_raw)
{
	return mmi_menu_list_callback(arg, slot_id, session_number,
								   title,sub_title,bottom,
								   item_count, items,
								   item_raw_length, items_raw, 0);
}


static void *ci_camthread(void *para)
{
	struct en50221_stdcam *stdcam;
	_ci_t *ci = (_ci_t*)para;

	while (!ci->camthread_quit) {

		stdcam = ci->stdcam;
		stdcam->poll(stdcam);

	}
	return NULL;
}

static AM_ErrorCode_t ci_open_stdcam(_ci_t *ci, int dev, int slot_num)
{
	int rc;
	struct en50221_stdcam *stdcam;

	UNUSED(dev);
	UNUSED(slot_num);

	// create transport layer
	ci->tl = en50221_tl_create(1, 16);
	if (ci->tl == NULL) {
		AM_DEBUG(1, "CI: Failed to create transport layer\n");
		return AM_CI_ERROR_PROTOCOL;
	}

	// create session layer
	ci->sl = en50221_sl_create(ci->tl, 16);
	if (ci->sl == NULL) {
		AM_DEBUG(1, "CI: Failed to create session layer\n");
		en50221_tl_destroy(ci->tl);
		return AM_CI_ERROR_PROTOCOL;
	}

	// create the stdcam instance
	stdcam = en50221_stdcam_create(ci->dev_id, ci->slot_no, ci->tl, ci->sl);
	if (stdcam == NULL) {
		AM_DEBUG(1, "CI: Failed to create stdcam\n");
		en50221_sl_destroy(ci->sl);
		en50221_tl_destroy(ci->tl);
		return AM_CI_ERROR_PROTOCOL;
	}
	ci->stdcam = stdcam;

	// hook up the AI callbacks
	if (stdcam->ai_resource) {
		en50221_app_ai_register_callback(stdcam->ai_resource, ci_ai_callback, ci);
	}

	// hook up the CA callbacks
	if (stdcam->ca_resource) {
		en50221_app_ca_register_info_callback(stdcam->ca_resource, ci_ca_info_callback, ci);
		en50221_app_ca_register_pmt_reply_callback(stdcam->ca_resource, ci_ca_pmt_relay_callback, ci);
	}

	// hook up the MMI callbacks
	if (stdcam->mmi_resource) {
		en50221_app_mmi_register_close_callback(stdcam->mmi_resource, ci_mmi_close_callback, ci);
		en50221_app_mmi_register_display_control_callback(stdcam->mmi_resource, ci_mmi_display_control_callback, ci);
		en50221_app_mmi_register_enq_callback(stdcam->mmi_resource, ci_mmi_enq_callback, ci);
		en50221_app_mmi_register_menu_callback(stdcam->mmi_resource, ci_mmi_menu_callback, ci);
		en50221_app_mmi_register_list_callback(stdcam->mmi_resource, ci_mmi_list_callback, ci);
	} else {
		AM_DEBUG(1, "CI: Warning!: CAM Menus are not supported by this interface hardware\n");
	}

	// start the cam thread
	rc = pthread_create(&ci->camthread, NULL, ci_camthread, (void*)ci);
	if (rc)
	{
		AM_DEBUG(1, "CI: camthread create fail: %s", strerror(rc));
		if (stdcam->destroy)
			stdcam->destroy(stdcam, 1);
		// destroy session layer
		en50221_sl_destroy(ci->sl);
		// destroy transport layer
		en50221_tl_destroy(ci->tl);
		return AM_CI_ERROR_CANNOT_CREATE_THREAD;
	}

	return AM_SUCCESS;
}

static AM_ErrorCode_t ci_close_stdcam(_ci_t *ci)
{
	pthread_t t;

	// shutdown the cam thread
	ci->camthread_quit = 1;
	t = ci->camthread;
	if (t != pthread_self())
		pthread_join(t, NULL);

	if (ci->stdcam && ci->stdcam->destroy)
		ci->stdcam->destroy(ci->stdcam, 1);
	ci->stdcam = NULL;

	// destroy session layer
	en50221_sl_destroy(ci->sl);
	// destroy transport layer
	en50221_tl_destroy(ci->tl);

	ci->sl = NULL;
	ci->tl = NULL;

	return AM_SUCCESS;
}


static void *ci_workthread(void *para)
{
	_ci_t *ci = (_ci_t*)para;

	return NULL;
}


AM_ErrorCode_t AM_CI_Open(int dev_no, int slot_no, const AM_CI_OpenPara_t *para, AM_CI_Handle_t *handle)
{
	int rc;
	AM_ErrorCode_t err = AM_SUCCESS;
	_ci_t *ci=NULL;

	if (!para || !handle)
		return AM_CI_ERROR_BAD_PARAM;

	err = get_ci_new(dev_no, slot_no, &ci);
	if (err != AM_SUCCESS)
		return err;

	pthread_mutex_lock(&ci->lock);

	if (ci->status != ci_status_closed)
		return AM_CI_ERROR_ALREADY_OPEN;

	memset(ci, 0, sizeof(_ci_t));
	ci->used = 1;
	ci->status = ci_status_open;
	ci->dev_id = dev_no;
	ci->slot_no = slot_no;

	pthread_mutex_unlock(&ci->lock);

	*handle = ci;

	return err;
}

AM_ErrorCode_t AM_CI_Close(AM_CI_Handle_t handle)
{
	pthread_t t;
	AM_ErrorCode_t err;
	_ci_t *ci;

	err = get_ci(handle, &ci);
	if (err != AM_SUCCESS)
		return err;

	pthread_mutex_lock(&ci->lock);

	if (ci->used_by_caman)
	{
		err = AM_CI_ERROR_USED_BY_CAMAN;
		goto quit;
	}

	if (ci->status >= ci_status_open)
	{

		if (ci->status == ci_status_started)
		{
			ci_close_stdcam(ci);
			ci_free_slots_info(ci);
		}
/*
		ci->workthread_quit = 1;
		t = ci->workthread;
		pthread_cond_signal(&ci->cond);

		if (t != pthread_self())
			pthread_join(t, NULL);

		pthread_cond_destroy(&ci->cond);
*/
		ci->status = ci_status_closed;
		ci->used = 0;
	}
quit:
	pthread_mutex_unlock(&ci->lock);
	return err;
}

AM_ErrorCode_t AM_CI_Start(AM_CI_Handle_t handle)
{
	AM_ErrorCode_t err;
	_ci_t *ci;

	err = get_ci(handle, &ci);
	if (err != AM_SUCCESS)
		return err;

	pthread_mutex_lock(&ci->lock);

	if (ci->status < ci_status_open) {
		err = AM_CI_ERROR_NOT_OPEN;
		goto quit;
	}

	if (ci->status > ci_status_open) {
		err = AM_CI_ERROR_ALREADY_START;
		goto quit;
	}

	ci->slot_info = calloc(sizeof(_slot_info_t), 1);
	assert(ci->slot_info);

	err = ci_open_stdcam(ci, ci->dev_id, ci->slot_no);
	if (err != AM_SUCCESS)
	{
		AM_DEBUG(1, "CI: open stdcam fail");
		free(ci->slot_info);
		err = AM_CI_ERROR_UNKOWN;
		goto quit;
	}

	ci->status = ci_status_started;

quit:
	pthread_mutex_unlock(&ci->lock);
	return err;
}

AM_ErrorCode_t AM_CI_Stop(AM_CI_Handle_t handle)
{
	AM_ErrorCode_t err=AM_SUCCESS;
	_ci_t *ci;

	err = get_ci(handle, &ci);
	if (err != AM_SUCCESS)
		return err;

	pthread_mutex_lock(&ci->lock);

	if (ci->used_by_caman)
	{
		err = AM_CI_ERROR_USED_BY_CAMAN;
		goto quit;
	}

	if (ci->status == ci_status_started) {
		ci_close_stdcam(ci);
		ci_free_slots_info(ci);
		ci->status = ci_status_open;
	}

quit:
	pthread_mutex_unlock(&ci->lock);
	return err;

}

AM_ErrorCode_t AM_CI_SetCallback(AM_CI_Handle_t handle, int cbid, AM_CI_CB_t *cb, void *arg)
{
	AM_ErrorCode_t err = AM_SUCCESS;
	_ci_t *ci;
	_slot_info_t *slot;

	err = get_ci(handle, &ci);
	if (err != AM_SUCCESS)
		return err;

	pthread_mutex_lock(&ci->lock);

	if (ci->status < ci_status_open)
	{
		err = AM_CI_ERROR_NOT_OPEN;
		goto quit;
	}

	switch (cbid)
	{
		#define SET(x) do{ci->x = cb->u.x; ci->x##_arg = arg;}while(0)

		case CBID_AI_CALLBACK:		SET(ai_cb);			break;
		case CBID_CA_INFO_CALLBACK:	SET(ca_info_cb);		break;
		case CBID_MMI_CLOSE_CALLBACK:	SET(mmi_close_cb);		break;
		case CBID_MMI_DISPLAY_CONTROL_CALLBACK: SET(mmi_display_control_cb);			break;
		case CBID_MMI_ENQ_CALLBACK:	SET(mmi_enq_cb);		break;
		case CBID_MMI_MENU_CALLBACK:	SET(mmi_menu_cb);	  	break;
		case CBID_MMI_LIST_CALLBACK:	SET(mmi_list_cb);      		break;
		default:
			err = AM_CI_ERROR_BAD_PARAM;
			break;

		#undef SET
	}

quit:
	pthread_mutex_unlock(&ci->lock);

	return err;
}

AM_ErrorCode_t AM_CI_App_ca_pmt(AM_CI_Handle_t handle, unsigned char *capmt, unsigned int size)
{
	AM_ErrorCode_t err = AM_SUCCESS;
	_ci_t *ci;

	if (!capmt || !size)
		return AM_CI_ERROR_BAD_PARAM;

	err = get_ci(handle, &ci);
	if (err != AM_SUCCESS)
		return err;

	pthread_mutex_lock(&ci->lock);

	if (ci->status < ci_status_started)
	{
		err = AM_CI_ERROR_NOT_START;
		goto quit;
	}

	if (ci->stdcam->ca_session_number != -1)
	{
		if ((ci->ca_resource_connected)) {
			if (en50221_app_ca_pmt(ci->stdcam->ca_resource, ci->stdcam->ca_session_number, capmt, size)) {
				AM_DEBUG(1, "CI: Failed to send PMT\n");
				err = AM_CI_ERROR_PROTOCOL;
			}
		}
		else
			err = AM_CI_ERROR_BAD_CAM;
	}
	else
		err=  AM_CI_ERROR_UNAVAILABLE;

quit:
	pthread_mutex_unlock(&ci->lock);

	return err;
}

AM_ErrorCode_t AM_CI_App_ai_entermenu(AM_CI_Handle_t handle)
{
	AM_ErrorCode_t err = AM_SUCCESS;
	_ci_t *ci;

	err = get_ci(handle, &ci);
	if (err != AM_SUCCESS)
		return err;

	pthread_mutex_lock(&ci->lock);

	if (ci->status < ci_status_started)
	{
		err = AM_CI_ERROR_NOT_START;
		goto quit;
	}

	if (ci->stdcam->ai_session_number != -1)
	{
		if ((ci->ca_resource_connected)) {
			if (en50221_app_ai_entermenu(ci->stdcam->ai_resource,
											ci->stdcam->ai_session_number))
				err = AM_CI_ERROR_PROTOCOL;
		}
		else
			err = AM_CI_ERROR_BAD_CAM;
	}
	else
		err=  AM_CI_ERROR_UNAVAILABLE;

quit:
	pthread_mutex_unlock(&ci->lock);

	return err;
}


AM_ErrorCode_t AM_CI_App_mmi_answ(AM_CI_Handle_t handle, int answer_id, char *answer, int size)
{
	AM_ErrorCode_t err = AM_SUCCESS;
	_ci_t *ci;

	err = get_ci(handle, &ci);
	if (err != AM_SUCCESS)
		return err;

	pthread_mutex_lock(&ci->lock);

	if (ci->status < ci_status_started)
	{
		err = AM_CI_ERROR_NOT_START;
		goto quit;
	}

	if (ci->stdcam->mmi_session_number != -1)
	{
		if (en50221_app_mmi_answ(ci->stdcam->mmi_resource,
									ci->stdcam->mmi_session_number,
									answer_id, (unsigned char*)answer, size))
				err = AM_CI_ERROR_PROTOCOL;
	}
	else
		err=  AM_CI_ERROR_UNAVAILABLE;

quit:
	pthread_mutex_unlock(&ci->lock);

	return err;
}

AM_ErrorCode_t AM_CI_App_mmi_menu_answ(AM_CI_Handle_t handle, int select)
{
	AM_ErrorCode_t err = AM_SUCCESS;
	_ci_t *ci;

	err = get_ci(handle, &ci);
	if (err != AM_SUCCESS)
		return err;

	pthread_mutex_lock(&ci->lock);

	if (ci->status < ci_status_started)
	{
		err = AM_CI_ERROR_NOT_START;
		goto quit;
	}

	if (ci->stdcam->mmi_session_number != -1)
	{
		if (en50221_app_mmi_menu_answ(ci->stdcam->mmi_resource,
							ci->stdcam->mmi_session_number,
							select))
				err = AM_CI_ERROR_PROTOCOL;
	}
	else
		err=  AM_CI_ERROR_UNAVAILABLE;

quit:
	pthread_mutex_unlock(&ci->lock);

	return err;
}

AM_ErrorCode_t AM_CI_App_mmi_close(AM_CI_Handle_t handle, int cmd_id, int delay)
{
	AM_ErrorCode_t err = AM_SUCCESS;
	_ci_t *ci;

	err = get_ci(handle, &ci);
	if (err != AM_SUCCESS)
		return err;

	pthread_mutex_lock(&ci->lock);

	if (ci->status < ci_status_started)
	{
		err = AM_CI_ERROR_NOT_START;
		goto quit;
	}

	if (ci->stdcam->mmi_session_number != -1)
	{
		if (en50221_app_mmi_close(ci->stdcam->mmi_resource,
							ci->stdcam->mmi_session_number,
							cmd_id, delay))
				err = AM_CI_ERROR_PROTOCOL;
	}
	else
		err=  AM_CI_ERROR_UNAVAILABLE;

quit:
	pthread_mutex_unlock(&ci->lock);

	return err;

}

/*user need to free the generated capmt with free() in the end */
AM_ErrorCode_t AM_CI_GenerateCAPMT(unsigned char *pmt, unsigned int pmt_size,
											unsigned char **capmt, unsigned int *capmt_size,
											int ca_list_management, int ca_pmt_cmd_id,
											int moveca)
{
#define CAPMT_SIZE 4096
	int size;
	unsigned char *ca_pmt;

	assert(pmt && pmt_size);
	assert(capmt && capmt_size);

	// parse section
	struct section *section = section_codec(pmt, pmt_size);
	if (section == NULL) {
		return AM_CI_ERROR_BAD_PMT;
	}

	// parse section_ext
	struct section_ext *section_ext = section_ext_decode(section, 0);
	if (section_ext == NULL) {
		return AM_CI_ERROR_BAD_PMT;
	}

	// parse PMT
	struct mpeg_pmt_section *spmt = mpeg_pmt_section_codec(section_ext);
	if (pmt == NULL) {
		return AM_CI_ERROR_BAD_PMT;
	}

	ca_pmt = malloc(CAPMT_SIZE);
	assert(ca_pmt);

	if ((size = en50221_ca_format_pmt(spmt, ca_pmt, CAPMT_SIZE,
				moveca, ca_list_management, ca_pmt_cmd_id)) < 0) {
		AM_DEBUG(1, "CI: Failed to format PMT\n");
		free(ca_pmt);
		return AM_CI_ERROR_BAD_PMT;
	}

	*capmt = ca_pmt;
	*capmt_size = size;

	return AM_SUCCESS;
}

AM_ErrorCode_t AM_CI_App_ai_enquiry(AM_CI_Handle_t handle)
{
	AM_ErrorCode_t err = AM_SUCCESS;
	_ci_t *ci;

	err = get_ci(handle, &ci);
	if (err != AM_SUCCESS)
		return err;

	pthread_mutex_lock(&ci->lock);

	if (ci->status < ci_status_started)
	{
		err = AM_CI_ERROR_NOT_START;
		goto quit;
	}

	if ((ci->stdcam->ai_session_number != -1)
		&& (ci->ca_resource_connected)) {
		if (en50221_app_ai_enquiry(ci->stdcam->ai_resource, ci->stdcam->ai_session_number)) {
			AM_DEBUG(1, "CI: Failed to enq app info\n");
			err = AM_CI_ERROR_PROTOCOL;
		}
	}
	else
		err = AM_CI_ERROR_UNAVAILABLE;

quit:
	pthread_mutex_unlock(&ci->lock);

	return err;
}

AM_ErrorCode_t AM_CI_App_ca_info_enq(AM_CI_Handle_t handle)
{
	AM_ErrorCode_t err = AM_SUCCESS;
	_ci_t *ci;

	err = get_ci(handle, &ci);
	if (err != AM_SUCCESS)
		return err;

	pthread_mutex_lock(&ci->lock);

	if (ci->status < ci_status_started)
	{
		err = AM_CI_ERROR_NOT_START;
		goto quit;
	}

	if ((ci->stdcam->ca_session_number != -1)
		&& (ci->ca_resource_connected)) {
		if (en50221_app_ca_info_enq(ci->stdcam->ca_resource, ci->stdcam->ca_session_number)) {
			AM_DEBUG(1, "CI: Failed to send PMT\n");
			err = AM_CI_ERROR_PROTOCOL;
		}
	}
	else
		err = AM_CI_ERROR_UNAVAILABLE;

quit:
	pthread_mutex_unlock(&ci->lock);

	return err;
}

AM_ErrorCode_t AM_CI_MatchCAID(AM_CI_Handle_t handle, unsigned int caid, int *match)
{
	AM_ErrorCode_t err = AM_SUCCESS;
	_ci_t *ci;

	assert(match);

	err = get_ci(handle, &ci);
	if (err != AM_SUCCESS)
		return err;

	pthread_mutex_lock(&ci->lock);

	if (ci->status < ci_status_started)
	{
		err = AM_CI_ERROR_NOT_START;
		goto quit;
	}

	*match = 0;
	if ((ci->stdcam->ca_session_number != -1)
		&& (ci->slot_info->ca_info.enable))
	{
		int i;
		for (i=0; i<ci->slot_info->ca_info.ca_id_count; i++)
			if (ci->slot_info->ca_info.ca_ids[i] == caid)
				*match = 1;
	}
	else
		err = AM_CI_ERROR_UNAVAILABLE;

quit:
	pthread_mutex_unlock(&ci->lock);

	return err;
}


int ci_caman_lock(AM_CI_Handle_t handle, int lock)
{
	AM_ErrorCode_t err = AM_SUCCESS;
	_ci_t *ci;

	err = get_ci(handle, &ci);
	if (err != AM_SUCCESS)
		return err;

	pthread_mutex_lock(&ci->lock);

	ci->used_by_caman = lock? 1 : 0;

	pthread_mutex_unlock(&ci->lock);

	return err;
}



//===============================================

#include "am_caman.h"
#include "ca_ci.h"
#include "ca_ci_internal.h"

AM_ErrorCode_t AM_CI_CAMAN_getCA(AM_CI_Handle_t handle, AM_CA_t **ca)
{
	AM_ErrorCode_t err = AM_SUCCESS;
	_ci_t *ci;

	assert(ca);

	err = get_ci(handle, &ci);
	if (err != AM_SUCCESS)
		return err;

	if (ci->status < ci_status_open)
		return AM_CI_ERROR_NOT_OPEN;

	ca_ci.arg = ca_ci_arg_create(ci);
	*ca = &ca_ci;

	return AM_SUCCESS;
}
