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

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "am_debug.h"
#include "am_ci.h"
#include "am_caman.h"

#include "ca_ci.h"
#include "ca_ci_internal.h"

typedef struct ca_ci_s ca_ci_t;

struct ca_ci_s {
	pthread_mutex_t lock;
	_ci_t *ci;
	int seenpmt;
	unsigned char *pmt;
	unsigned int pmt_size;
	int send_enable;
	char *send_name;
	int (*send_msg)(char *name, AM_CA_Msg_t *msg);
	unsigned int ca_id_count;
	unsigned short *ca_ids;
};

static int _ca_ci_sendmsg(ca_ci_t *ca_ci, int type, int dst, ca_ci_msg_t *data, int size)
{
	AM_CA_Msg_t camsg;
	int ret=0;

	pthread_mutex_lock(&ca_ci->lock);

	if (ca_ci->send_enable && ca_ci->send_msg)
	{
		camsg.type = type;
		camsg.dst = dst;
		camsg.data = (unsigned char*)data;
		camsg.len = size;

		if (ca_ci->send_msg(ca_ci->send_name, &camsg) != 0)
			ret = -1;
	}

	pthread_mutex_unlock(&ca_ci->lock);
	return ret;
}
static int _ai_callback(void *arg, uint8_t slot_id, uint16_t session_number,
			     uint8_t application_type, uint16_t application_manufacturer, uint16_t manufacturer_code,
			     uint8_t menu_string_length, uint8_t *menu_string)
{
	ca_ci_t *ca_ci = (ca_ci_t*)arg;
	int msg_size = sizeof(ca_ci_msg_t) + sizeof(ca_ci_appinfo_t) + menu_string_length*sizeof(uint8_t);
	ca_ci_msg_t *msgdata;
	ca_ci_appinfo_t *appinfo;

	UNUSED(slot_id);
	UNUSED(session_number);

	msgdata = malloc(msg_size);
	assert(msgdata);

	msgdata->type = ca_ci_msg_type_appinfo;
	appinfo = (ca_ci_appinfo_t*)msgdata->msg;
	appinfo->application_type = application_type;
	appinfo->application_manufacturer = application_manufacturer;
	appinfo->manufacturer_code = manufacturer_code;
	appinfo->menu_string_length = menu_string_length;
	memcpy(appinfo->menu_string, menu_string, appinfo->menu_string_length);

	if (_ca_ci_sendmsg(ca_ci, 0, AM_CA_MSG_DST_APP, msgdata, msg_size) != 0)
		free(msgdata);

	return 0;
}

static int _ca_info_callback(void *arg, uint8_t slot_id, uint16_t session_number, uint32_t ca_id_count, uint16_t *ca_ids)
{
	ca_ci_t *ca_ci = (ca_ci_t*)arg;
	int msg_size = sizeof(ca_ci_msg_t) + sizeof(ca_ci_cainfo_t) + ca_id_count*sizeof(uint16_t);
	ca_ci_msg_t *msgdata;
	ca_ci_cainfo_t *cainfo;

	UNUSED(slot_id);
	UNUSED(session_number);

	msgdata = malloc(msg_size);
	assert(msgdata);

	msgdata->type = ca_ci_msg_type_cainfo;
	cainfo = (ca_ci_cainfo_t*)msgdata->msg;
	cainfo->ca_id_count = ca_id_count;
	memcpy(cainfo->ca_ids, ca_ids, cainfo->ca_id_count*sizeof(uint16_t));


	if (_ca_ci_sendmsg(ca_ci, 0, AM_CA_MSG_DST_APP, msgdata, msg_size) != 0)
		free(msgdata);

	return 0;
}

static int _mmi_close_callback(void *arg, uint8_t slot_id, uint16_t session_number,
				    uint8_t cmd_id, uint8_t delay)
{
	ca_ci_t *ca_ci = (ca_ci_t*)arg;
	int msg_size = sizeof(ca_ci_msg_t) + sizeof(ca_ci_mmi_close_t);
	ca_ci_msg_t *msgdata;
	ca_ci_mmi_close_t *mmiclose;

	UNUSED(slot_id);
	UNUSED(session_number);

	msgdata = malloc(msg_size);
	assert(msgdata);

	msgdata->type = ca_ci_msg_type_mmi_close;
	mmiclose = (ca_ci_mmi_close_t*)msgdata->msg;
	mmiclose->cmd_id = cmd_id;
	mmiclose->delay = delay;

	if (_ca_ci_sendmsg(ca_ci, 0, AM_CA_MSG_DST_APP, msgdata, msg_size) != 0)
		free(msgdata);

	return 0;
}

static int _mmi_display_control_callback(void *arg, uint8_t slot_id, uint16_t session_number,
					      uint8_t cmd_id, uint8_t mmi_mode)
{
	ca_ci_t *ca_ci = (ca_ci_t*)arg;
	int msg_size = sizeof(ca_ci_msg_t) + sizeof(ca_ci_mmi_display_control_t);
	ca_ci_msg_t *msgdata;
	ca_ci_mmi_display_control_t *dc;

	UNUSED(slot_id);
	UNUSED(session_number);

	msgdata = malloc(msg_size);
	assert(msgdata);

	msgdata->type = ca_ci_msg_type_mmi_display_control;
	dc = (ca_ci_mmi_display_control_t*)msgdata->msg;
	dc->cmd_id = cmd_id;
	dc->mmi_mode = mmi_mode;

	if (_ca_ci_sendmsg(ca_ci, 0, AM_CA_MSG_DST_APP, msgdata, msg_size) != 0)
		free(msgdata);

	return 0;
}
static int _mmi_enq_callback(void *arg, uint8_t slot_id, uint16_t session_number,
				  uint8_t blind_answer, uint8_t expected_answer_length,
				  uint8_t *text, uint32_t text_size)
{
	ca_ci_t *ca_ci = (ca_ci_t*)arg;
	int msg_size = sizeof(ca_ci_msg_t) + sizeof(ca_ci_mmi_enq_t) + text_size*sizeof(uint8_t);
	ca_ci_msg_t *msgdata;
	ca_ci_mmi_enq_t *enq;

	UNUSED(slot_id);
	UNUSED(session_number);

	msgdata = malloc(msg_size);
	assert(msgdata);

	msgdata->type = ca_ci_msg_type_mmi_enq;
	enq = (ca_ci_mmi_enq_t*)msgdata->msg;
	enq->blind_answer = blind_answer;
	enq->expected_answer_length = expected_answer_length;
	enq->text_size = text_size;
	memcpy(enq->text, text, enq->text_size);

	if (_ca_ci_sendmsg(ca_ci, 0, AM_CA_MSG_DST_APP, msgdata, msg_size) != 0)
		free(msgdata);

	return 0;
}


static inline int _copy_mmi_text(void *to, app_mmi_text_t *from) \
{
	unsigned char *t = (unsigned char *)(to);
	int size = sizeof(from->text_size);
	memcpy(t, &from->text_size, size);
	memcpy(t+size, from->text, from->text_size);
	return size + from->text_size;
}


static int __mmi_menu_list_callback(void *arg, uint8_t slot_id, uint16_t session_number,
				   app_mmi_text_t *title,
				   app_mmi_text_t *sub_title,
				   app_mmi_text_t *bottom,
				   uint32_t item_count, app_mmi_text_t *items,
				   uint32_t item_raw_length, uint8_t *items_raw, int ismenu)
{
	ca_ci_t *ca_ci = (ca_ci_t*)arg;
	int msg_size=0;
	ca_ci_msg_t *msgdata;
	unsigned char *menu, *ptr;
	unsigned int off=0, i;

	UNUSED(session_number);
	UNUSED(slot_id);

#define SIZE_OF(text) ((text)->text_size + sizeof((text)->text_size))

	msg_size = sizeof(ca_ci_msg_t) + SIZE_OF(title) + SIZE_OF(sub_title) + SIZE_OF(bottom);
	msg_size += sizeof(item_count);
	for (i=0; i<item_count; i++)
		msg_size += SIZE_OF(&items[i]);
	msg_size += sizeof(item_raw_length);
	msg_size += item_raw_length;

	msgdata = malloc(msg_size);
	assert(msgdata);

	msgdata->type = (ismenu)? ca_ci_msg_type_mmi_menu : ca_ci_msg_type_mmi_list;
	menu = ptr = msgdata->msg;

	ptr += _copy_mmi_text(ptr, title);
	ptr += _copy_mmi_text(ptr, sub_title);
	ptr += _copy_mmi_text(ptr, bottom);

	memcpy(ptr, &item_count, sizeof(item_count));
	ptr += sizeof(item_count);
	for (i=0; i<item_count; i++)
		ptr += _copy_mmi_text(ptr, &items[i]);

	memcpy(ptr, &item_raw_length, sizeof(item_raw_length));
	ptr += sizeof(item_raw_length);
	memcpy(ptr, items_raw, item_raw_length);

	assert((ptr-menu+item_raw_length) == msg_size);

	//debug_dump(msgdata, msg_size);
	if (_ca_ci_sendmsg(ca_ci, 0, AM_CA_MSG_DST_APP, msgdata, msg_size) != 0)
		free(msgdata);

	return 0;
}

static int _mmi_menu_callback(void *arg, uint8_t slot_id, uint16_t session_number,
				   app_mmi_text_t *title,
				   app_mmi_text_t *sub_title,
				   app_mmi_text_t *bottom,
				   uint32_t item_count, app_mmi_text_t *items,
				   uint32_t item_raw_length, uint8_t *items_raw)
{
	return __mmi_menu_list_callback(arg, slot_id, session_number,
					title, sub_title, bottom,
					item_count, items,
					item_raw_length, items_raw, 1);
}
static int _mmi_list_callback(void *arg, uint8_t slot_id, uint16_t session_number,
				   app_mmi_text_t *title,
				   app_mmi_text_t *sub_title,
				   app_mmi_text_t *bottom,
				   uint32_t item_count, app_mmi_text_t *items,
				   uint32_t item_raw_length, uint8_t *items_raw)
{
	return __mmi_menu_list_callback(arg, slot_id, session_number,
					title, sub_title, bottom,
					item_count, items,
					item_raw_length, items_raw, 0);
}


static int ca_ci_open(void *arg, AM_CAMAN_Ts_t *ts)
{
	ca_ci_t *ca_ci = (ca_ci_t*)arg;
	AM_CI_Handle_t dev = ca_ci->ci;
	AM_CI_CB_t cb;
	AM_ErrorCode_t ret;

	UNUSED(ts);

	AM_DEBUG(1, "\t-->|ca ci open, arg[%p]", arg);

	pthread_mutex_lock(&ca_ci->lock);
	ca_ci->seenpmt = 0;
	ca_ci->send_msg = NULL;

	cb.u.ai_cb = _ai_callback;
	AM_TRY_FINAL(AM_CI_SetCallback(dev, CBID_AI_CALLBACK, &cb, ca_ci));

	cb.u.ca_info_cb = _ca_info_callback;
	AM_TRY_FINAL(AM_CI_SetCallback(dev, CBID_CA_INFO_CALLBACK, &cb, ca_ci));

	cb.u.mmi_close_cb = _mmi_close_callback;
	AM_TRY_FINAL(AM_CI_SetCallback(dev, CBID_MMI_CLOSE_CALLBACK, &cb, ca_ci));

	cb.u.mmi_display_control_cb = _mmi_display_control_callback;
	AM_TRY_FINAL(AM_CI_SetCallback(dev, CBID_MMI_DISPLAY_CONTROL_CALLBACK, &cb, ca_ci));

	cb.u.mmi_enq_cb = _mmi_enq_callback;
	AM_TRY_FINAL(AM_CI_SetCallback(dev, CBID_MMI_ENQ_CALLBACK, &cb, ca_ci));

	cb.u.mmi_menu_cb = _mmi_menu_callback;
	AM_TRY_FINAL(AM_CI_SetCallback(dev, CBID_MMI_MENU_CALLBACK, &cb, ca_ci));

	cb.u.mmi_list_cb = _mmi_list_callback;
	AM_TRY_FINAL(AM_CI_SetCallback(dev, CBID_MMI_LIST_CALLBACK, &cb, ca_ci));

	AM_CI_Start(dev);

	ci_caman_lock(dev, 1);

final:
	pthread_mutex_unlock(&ca_ci->lock);
	return ret;
}

static int ca_ci_close(void *arg)
{
	ca_ci_t *ca_ci = (ca_ci_t*)arg;
	AM_CI_Handle_t dev = ca_ci->ci;
	AM_CI_CB_t cb;

	AM_DEBUG(1, "\t-->|ca ci close");

	pthread_mutex_lock(&ca_ci->lock);

	cb.u.ai_cb = NULL;

	#define SET_CB_NULL(id) AM_CI_SetCallback(dev, (id), &cb, NULL)

	SET_CB_NULL(CBID_AI_CALLBACK);
	SET_CB_NULL(CBID_CA_INFO_CALLBACK);
	SET_CB_NULL(CBID_MMI_CLOSE_CALLBACK);
	SET_CB_NULL(CBID_MMI_DISPLAY_CONTROL_CALLBACK);
	SET_CB_NULL(CBID_MMI_ENQ_CALLBACK);
	SET_CB_NULL(CBID_MMI_MENU_CALLBACK);
	SET_CB_NULL(CBID_MMI_LIST_CALLBACK);

	ci_caman_lock(dev, 0);

	AM_CI_Stop(dev);

	pthread_mutex_unlock(&ca_ci->lock);

	return 0;
}

static int ca_ci_camatch(void *arg, unsigned int caid)
{
	ca_ci_t *ca_ci = (ca_ci_t*)arg;
	AM_CI_Handle_t dev = ca_ci->ci;
	int match=0;
	AM_ErrorCode_t err;

	AM_DEBUG(1, "\t-->|ca ci camatch");

	pthread_mutex_lock(&ca_ci->lock);

	err = AM_CI_MatchCAID(dev, caid, &match);

	pthread_mutex_unlock(&ca_ci->lock);

	return ((err==AM_SUCCESS) && match);
}

static int ca_ci_new_cat(void *arg, unsigned char *cat, unsigned int size)
{
	UNUSED(arg);
	UNUSED(cat);
	UNUSED(size);

	AM_DEBUG(1, "\t-->|ca ci new cat");

	return 0;
}


static int ca_ci_startpmt(void *arg, int service_id, unsigned char *pmt, unsigned int size)
{
	ca_ci_t *ca_ci = (ca_ci_t*)arg;
	AM_CI_Handle_t dev = ca_ci->ci;
	AM_ErrorCode_t ret=AM_SUCCESS;
	uint8_t *capmt;
	unsigned int capmt_size;

	AM_DEBUG(1, "\t-->|ca ci start capmt srv[%d]", service_id);

	AM_DEBUG(1, "\t-->|Received new PMT - sending to CAM...\n");

	pthread_mutex_lock(&ca_ci->lock);

	// translate it into a CA PMT
	int listmgmt = AM_CI_CA_LIST_MANAGEMENT_ONLY;
	if (ca_ci->seenpmt) {
		listmgmt = AM_CI_CA_LIST_MANAGEMENT_UPDATE;
	}
	ca_ci->seenpmt = 1;

	AM_TRY_FINAL(AM_CI_GenerateCAPMT(pmt, size, &capmt, &capmt_size, listmgmt, AM_CI_CA_PMT_CMD_ID_OK_DESCRAMBLING, 0));
	AM_TRY_FINAL(AM_CI_App_ca_pmt(dev, capmt, capmt_size));

	ca_ci->pmt_size = size;
	//assert(ca_ci->pmt = realloc(ca_ci->pmt, size));
	ca_ci->pmt = realloc(ca_ci->pmt, size);
	assert(ca_ci->pmt);
	memcpy(ca_ci->pmt, pmt, size);

final:
	if (capmt)
		free(capmt);
	pthread_mutex_unlock(&ca_ci->lock);

	return (ret==AM_SUCCESS)? 0 : -1;
}

static int ca_ci_stoppmt(void *arg, int service_id)
{
	ca_ci_t *ca_ci = (ca_ci_t*)arg;
	AM_CI_Handle_t dev = ca_ci->ci;
	uint8_t *capmt=NULL;
	unsigned int size;
	AM_ErrorCode_t ret=AM_SUCCESS;

	AM_DEBUG(1, "\t-->|ci ca stop capmt srv[%d]", service_id);

	pthread_mutex_lock(&ca_ci->lock);
	if (ca_ci->pmt)
	{
		// translate it into a CA PMT
		int listmgmt = AM_CI_CA_LIST_MANAGEMENT_ONLY;
		if (ca_ci->seenpmt) {
			listmgmt = AM_CI_CA_LIST_MANAGEMENT_UPDATE;
		}
		ca_ci->seenpmt = 1;

		AM_TRY_FINAL(AM_CI_GenerateCAPMT(ca_ci->pmt, ca_ci->pmt_size, &capmt, &size, listmgmt, AM_CI_CA_PMT_CMD_ID_NOT_SELECTED, 0));
		AM_TRY_FINAL(AM_CI_App_ca_pmt(dev, capmt, size));
	}

final:
	if (capmt)
		free(capmt);
	pthread_mutex_unlock(&ca_ci->lock);

	return (ret==AM_SUCCESS)? 0 : -1;;
}

static int ca_ci_ts_changed(void *arg)
{
	UNUSED(arg);
	AM_DEBUG(1, "\t-->|ci ca ts changed");
	return 0;
}

static int ca_ci_enable(void *arg, int enable)
{
	ca_ci_t *ca_ci = (ca_ci_t*)arg;

	AM_DEBUG(1, "\t-->|ca ci enable [%s]", enable? "ENABLE" : "DISABLE");
	pthread_mutex_lock(&ca_ci->lock);
	ca_ci->send_enable  = enable ? 1 : 0;
	pthread_mutex_unlock(&ca_ci->lock);
	return 0;
}

static int ca_ci_register_msg_send(void *arg, char *name, int (*send_msg)(char *name, AM_CA_Msg_t *msg))
{
	ca_ci_t *ca_ci = (ca_ci_t*)arg;

	AM_DEBUG(1, "\t-->|ca ci msg func [%p] ", send_msg);

	pthread_mutex_lock(&ca_ci->lock);
	ca_ci->send_name = name;
	ca_ci->send_msg = send_msg;
	ca_ci->send_enable = 1;
	pthread_mutex_unlock(&ca_ci->lock);
	return 0;
}

static void ca_ci_free_msg(void *arg, AM_CA_Msg_t *msg)
{
	UNUSED(arg);

	AM_DEBUG(1, "\t-->|ca ci free msg");

	if (msg && msg->data)
		free(msg->data);
	return;
}

static int ca_ci_msg_receive(void *arg, AM_CA_Msg_t *msg)
{
	ca_ci_t *ca_ci = (ca_ci_t*)arg;
	AM_CI_Handle_t dev = ca_ci->ci;
	ca_ci_msg_t *cimsg = (ca_ci_msg_t *)msg->data;
	int ret = 0;

	if (!msg->data || !msg->len)
		return 0;

	AM_DEBUG(1, "\t-->|ca ci msg receive[type:0x%x]", cimsg->type);

	switch (cimsg->type)
	{
		case ca_ci_msg_type_enter_menu:
			AM_DEBUG(1, "\t-->|ca ci msg [ca_ci_msg_type_enter_menu]");
			ret = AM_CI_App_ai_entermenu(dev);
			AM_DEBUG(1, "entermenu ret = 0x%x", ret);
			break;

		case ca_ci_msg_type_answer:
			AM_DEBUG(1, "\t-->|ca ci msg [ca_ci_msg_type_answer]");
			{
				ca_ci_answer_enq_t *answ_enq = (ca_ci_answer_enq_t *)cimsg->msg;
				ret = AM_CI_App_mmi_answ(dev, answ_enq->answer_id, answ_enq->answer, answ_enq->size);
			}
			break;

		case ca_ci_msg_type_answer_menu:
			AM_DEBUG(1, "\t-->|ca ci msg [ca_ci_msg_type_answer_menu]");
			{
				ca_ci_answer_menu_t *answ_menu = (ca_ci_answer_menu_t *)cimsg->msg;
				ret = AM_CI_App_mmi_menu_answ(dev, answ_menu->select);
			}
			break;

		case ca_ci_msg_type_close_mmi:
			AM_DEBUG(1, "\t-->|ca ci msg [ca_ci_msg_type_close_mmi]");
			{
				ca_ci_close_mmi_t *close_mmi = (ca_ci_close_mmi_t*)cimsg->msg;
				ret = AM_CI_App_mmi_close(dev, close_mmi->cmd_id, close_mmi->delay);
			}
			break;

		case ca_ci_msg_type_set_pmt:
			AM_DEBUG(1, "\t-->|ca ci msg [ca_ci_msg_type_set_pmt]");
			{
				ca_ci_set_pmt_t *pmt = (ca_ci_set_pmt_t*)cimsg->msg;
				unsigned char *capmt;
				unsigned int capmt_size;
				AM_CI_GenerateCAPMT(pmt->pmt, pmt->length, &capmt, &capmt_size, pmt->list_management, pmt->cmd_id, 1);
				ret = AM_CI_App_ca_pmt(dev, capmt, capmt_size);
				free(capmt);
			}
			break;

		case ca_ci_msg_type_get_appinfo:
			AM_DEBUG(1, "\t-->|ca ci msg [ca_ci_msg_type_get_appinfo]");
			ret = AM_CI_App_ai_enquiry(dev);
			break;
		case ca_ci_msg_type_get_cainfo:
			AM_DEBUG(1, "\t-->|ca ci msg [ca_ci_msg_type_get_cainfo]");
			ret = AM_CI_App_ca_info_enq(dev);
			break;

		default:
			break;
	}

	return ret;
}

static void ca_ci_arg_destroy(void *arg)
{
	AM_DEBUG(1, "\t-->|ca ci arg destroy");
	if (arg)
	{
		ca_ci_t *ca_ci = (ca_ci_t *)arg;
		if (ca_ci->pmt)
			free(ca_ci->pmt);
		free(arg);
	}
}

void* ca_ci_arg_create(_ci_t *ci)
{
	pthread_mutexattr_t mta;
	ca_ci_t *ca_ci = malloc(sizeof(ca_ci_t));
	assert(ca_ci);

	AM_DEBUG(1, "\t-->|ca ci arg create");

	memset(ca_ci, 0, sizeof(ca_ci_t));
	ca_ci->ci = ci;
	ca_ci->seenpmt = 0;
	ca_ci->send_msg = NULL;

	pthread_mutexattr_init(&mta);
//	pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_RECURSIVE_NP);
	pthread_mutex_init(&ca_ci->lock, &mta);
	pthread_mutexattr_destroy(&mta);

	return (void*)ca_ci;
}


AM_CA_t ca_ci = {
	.type = AM_CA_TYPE_CI,

	.arg = NULL,/*feed this*/
	.arg_destroy = ca_ci_arg_destroy,

	.ops = {
		.open = ca_ci_open,
		.close= ca_ci_close,
		.camatch = ca_ci_camatch,

		.ts_changed = ca_ci_ts_changed,

		.new_cat = ca_ci_new_cat,

		.start_pmt = ca_ci_startpmt,
		.stop_pmt = ca_ci_stoppmt,

		.enable = ca_ci_enable,

		.register_msg_send = ca_ci_register_msg_send,
		.free_msg = ca_ci_free_msg,
		.msg_receive = ca_ci_msg_receive,
	}
};
