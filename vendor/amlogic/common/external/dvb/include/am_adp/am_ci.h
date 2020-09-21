/***************************************************************************
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
/**\file
 * \brief am ci module
 *
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-08-06: create the document
 ***************************************************************************/

#ifndef _AM_CI_H_
#define _AM_CI_H_

#include "am_caman.h"

#ifdef __cplusplus
extern "C" {
#endif
/**\brief Error code of the ci module*/
enum AM_CI_ErrorCode
{
	AM_CI_ERROR_BASE=AM_ERROR_BASE(AM_MOD_CI),
	AM_CI_ERROR_INVALID_DEV_NO, 							/**< Invalid device number*/
	AM_CI_ERROR_BAD_PARAM,										/**< Invalid parameter*/
	AM_CI_ERROR_NOT_OPEN,											/**< The device is not yet open */
	AM_CI_ERROR_NOT_START,										/**< The device is already start*/
	AM_CI_ERROR_ALREADY_OPEN,									/**< The device is not yet open*/
	AM_CI_ERROR_ALREADY_START,								/**< The device is already start*/
	AM_CI_ERROR_CANNOT_CREATE_THREAD,					/**< Cannot creat thread*/
	AM_CI_ERROR_USED_BY_CAMAN,								/**< CAMAN error */
	AM_CI_ERROR_PROTOCOL,											/**< Protocol error*/
	AM_CI_ERROR_BAD_PMT,											/**< PMT is bad error*/
	AM_CI_ERROR_MAX_DEV,											/**< already open max device,need close other device*/
	AM_CI_ERROR_BAD_CAM,											/**< Cam card is bad*/
	AM_CI_ERROR_UNAVAILABLE,									/**< ci unavailable error*/
	AM_CI_ERROR_UNKOWN,												/**< Unkown error*/
};
/**\brief callback id of the ci module*/
enum AM_CI_CBID
{
	CBID_AI_CALLBACK,											/**< ai callback ID*/
	CBID_CA_INFO_CALLBACK ,								/**< ca info callback ID*/
	CBID_MMI_CLOSE_CALLBACK ,							/**< mmi close callback ID*/
	CBID_MMI_DISPLAY_CONTROL_CALLBACK ,		/**< mmi display control callback ID*/
	CBID_MMI_ENQ_CALLBACK ,								/**< mmi enquiry callback ID*/
	CBID_MMI_MENU_CALLBACK ,							/**< mmi menu callback ID*/
	CBID_MMI_LIST_CALLBACK ,							/**< mmi list callback ID*/
};
/**\brief mmi answer id */
enum AM_CI_MMI_ANSWER_ID
{
	AM_CI_MMI_ANSW_CANCEL = 0x00,	/**< mmi cancel*/
	AM_CI_MMI_ANSW_ANSWER = 0x01, /**< mmi answer*/
};
/**\brief close mmi cmd id */
enum AM_CI_MMI_CLOSE_MMI_CMD_ID
{
	AM_CI_MMI_CLOSE_MMI_CMD_ID_IMMEDIATE = 0x00, /**< send mmi close cmd immediately*/
	AM_CI_MMI_CLOSE_MMI_CMD_ID_DELAY = 0x01,		 /**< send mmi close cmd delay*/
};
/**\brief ca pmt list management */
enum AM_CI_CA_LIST_MANAGEMENT
{
	AM_CI_CA_LIST_MANAGEMENT_MORE =    0x00,	/**< has more pmt*/
	AM_CI_CA_LIST_MANAGEMENT_FIRST =   0x01,	/**< the first one pmt*/
	AM_CI_CA_LIST_MANAGEMENT_LAST =    0x02,	/**< the last one pmt*/
	AM_CI_CA_LIST_MANAGEMENT_ONLY =    0x03,	/**< only for single ca pmt*/
	AM_CI_CA_LIST_MANAGEMENT_ADD =     0x04,	/**< add for already existing programme */
	AM_CI_CA_LIST_MANAGEMENT_UPDATE =  0x05,	/**< update means that ca pmt already existing,is sent again*/
};
/**\brief ca pmt cmd id */
enum AM_CI_CA_PMT_CMD_ID
{
	AM_CI_CA_PMT_CMD_ID_OK_DESCRAMBLING =  0x01,	/**< application can start descrambling*/
	AM_CI_CA_PMT_CMD_ID_OK_MMI =           0x02,	/**< application can send mmi dialogue but shall not satrt descrambling*/
	AM_CI_CA_PMT_CMD_ID_QUERY =            0x03,	/**< host expect to receive ca pmt reply,the application is not allowen to start des and mmi dialogue*/
	AM_CI_CA_PMT_CMD_ID_NOT_SELECTED =     0x04,	/**< host no longer requires that ca applition to descramble the service*/
};
/**\brief CI handle type*/
typedef void* AM_CI_Handle_t;
/**\brief CI open  parameter*/
typedef struct
{
	int foo;
} AM_CI_OpenPara_t;
/**\brief ci mmi text info struct*/
typedef struct  {
	uint8_t *text;			/**< mmi text content*/
	uint32_t text_size;	/**< mmi text content length*/
} app_mmi_text_t;
/**\brief ci application info callback
 * \param[in] arg callback used private data
 * \param[in] slot_id ci slot id
 * \param[in] session_number	ci session number
 * \param[in] application_type	application type
 * \param[in] application_manufacturer application manufacturer
 * \param[in] menu_string_length	menu string length
 * \param[in] menu_string menu string
 * \retval AM_SUCCESS On success
 * \return Error code
 */
typedef int (*ai_callback)(void *arg, uint8_t slot_id, uint16_t session_number,
			     uint8_t application_type, uint16_t application_manufacturer,
			     uint16_t manufacturer_code, uint8_t menu_string_length,
			     uint8_t *menu_string);
/**\brief ci info callback
 * \param[in] arg callback used private data
 * \param[in] slot_id ci slot id
 * \param[in] session_number	ci session number
 * \param[in] ca_id_count	ca id count number
 * \param[in] ca_ids ca id array
 * \retval AM_SUCCESS On success
 * \return Error code
 */
typedef int (*ca_info_callback)(void *arg, uint8_t slot_id, uint16_t session_number, uint32_t ca_id_count, uint16_t *ca_ids);
/**\brief mmi close callback
 * \param[in] arg callback used private data
 * \param[in] slot_id ci slot id
 * \param[in] session_number	ci session number
 * \param[in] cmd_id	see AM_CI_MMI_CLOSE_MMI_CMD_ID
 * \param[in] delay delay time
 * \retval AM_SUCCESS On success
 * \return Error code
 */
typedef int (*mmi_close_callback)(void *arg, uint8_t slot_id, uint16_t session_number, uint8_t cmd_id, uint8_t delay);
/**\brief mmi display control callback
 * \param[in] arg callback used private data
 * \param[in] slot_id ci slot id
 * \param[in] session_number	ci session number
 * \param[in] cmd_id	see MMI_DISPLAY_CONTROL_CMD_ID* in en50221_app_mmi.h
 * \param[in] mmi_mode see MMI_MODE_* in en50221_app_mmi.h
 * \retval AM_SUCCESS On success
 * \return Error code
 */
typedef int (*mmi_display_control_callback)(void *arg, uint8_t slot_id, uint16_t session_number,
					      uint8_t cmd_id, uint8_t mmi_mode);
/**\brief mmi enq callback
 * \param[in] arg callback used private data
 * \param[in] slot_id ci slot id
 * \param[in] session_number	ci session number
 * \param[in] blind_answer	set to 1 menus that user input has not to be displayed
 * \param[in] expected_answer_length expected length,if set to FF if unkown
 * \param[in] text	input text string
 * \param[in] text_size input text length
 * \retval AM_SUCCESS On success
 * \return Error code
 */
typedef int (*mmi_enq_callback)(void *arg, uint8_t slot_id, uint16_t session_number,
				  uint8_t blind_answer, uint8_t expected_answer_length,
				  uint8_t *text, uint32_t text_size);
/**\brief mmi menu callback
 * \param[in] arg callback used private data
 * \param[in] slot_id ci slot id
 * \param[in] session_number	ci session number
 * \param[in] title	menu title string
 * \param[in] sub_title menu subtitle string
 * \param[in] bottom	menu bottom string
 * \param[in] item_count menu list item count
 * \param[in] items menu items string
 * \param[in] item_raw_length menu item raw string length
 * \param[in] items_raw menu item raw string
 * \retval AM_SUCCESS On success
 * \return Error code
 */
typedef int (*mmi_menu_callback)(void *arg, uint8_t slot_id, uint16_t session_number,
				   app_mmi_text_t *title,
				   app_mmi_text_t *sub_title,
				   app_mmi_text_t *bottom,
				   uint32_t item_count, app_mmi_text_t *items,
				   uint32_t item_raw_length, uint8_t *items_raw);

/**\brief ca callback struct */
typedef struct {
	union{
		ai_callback ai_cb;																		/**< ai callback*/
		ca_info_callback ca_info_cb;													/**< ca info callback*/
		mmi_close_callback mmi_close_cb;											/**< mmi close callback*/
		mmi_display_control_callback mmi_display_control_cb;	/**< mmi display callback*/
		mmi_enq_callback mmi_enq_cb;													/**< mmi enq callback*/
		mmi_menu_callback mmi_menu_cb;												/**< mmi menu callback*/
		mmi_menu_callback mmi_list_cb;												/**< mmi list callback*/
	}u;
}AM_CI_CB_t;
/**\brief Open ci device
 * \param [in] dev_no ci device number
 * \param [in] slot_no Serviceid
 * \param [in] para ci open parameter
 * \param [out] handle ci open handle
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_CI_Open(int dev_no, int slot_no, const AM_CI_OpenPara_t *para, AM_CI_Handle_t *handle);
/**\brief Close ci device
 * \param [in] handle ci opened handle
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_CI_Close(AM_CI_Handle_t handle);
/**\brief start ci
 * \param [in] handle ci opened handle
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_CI_Start(AM_CI_Handle_t handle);
/**\brief stop ci
 * \param [in] handle ci opened handle
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_CI_Stop(AM_CI_Handle_t handle);

/*set callbacks before start() or you may miss callbacks*/
/**\brief set ci callback
 * \param [in] handle ci opened handle
 * \param [in] cbid callback id,see AM_CI_CBID enum
 * \param [in] cb ci callback function
 * \param [in] arg ci callback private data
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_CI_SetCallback(AM_CI_Handle_t handle, int cbid, AM_CI_CB_t *cb, void *arg);

/**\brief seen pmt to cam card
 * \param [in] handle ci opened handle
 * \param [in] capmt pmt data buffer
 * \param [in] size pmt buffer length
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_CI_App_ca_pmt(AM_CI_Handle_t handle, unsigned char *capmt, unsigned int size);
/**\brief ci enter menu
 * \param [in] handle ci opened handle
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_CI_App_ai_entermenu(AM_CI_Handle_t handle);
/*answer_id see:AM_CI_MMI_ANSWER_ID*/

/**\brief ci mmi answer
 * \param [in] handle ci opened handle
 * \param [in] answer_id ci answer id
 * \param [in] answer ci answer string
 * \param [in] size ci answer size
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_CI_App_mmi_answ(AM_CI_Handle_t handle, int answer_id, char *answer, int size);
/**\brief ci mmi slect menu
 * \param [in] handle ci opened handle
 * \param [in] select ci selected menu item index
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_CI_App_mmi_menu_answ(AM_CI_Handle_t handle, int select);
/**\brief ci mmi close
 * \param [in] handle ci opened handle
 * \param [in] cmd_id mmi close cmd id,see AM_CI_MMI_CLOSE_MMI_CMD_ID
 * \param [in] delay delay time
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_CI_App_mmi_close(AM_CI_Handle_t handle, int cmd_id, int delay);

/*infomations enquired will be repled in the callback*/

/**\brief ci enquie ca info
 * \param [in] handle ci opened handle
 * \return Error code
 */
extern AM_ErrorCode_t AM_CI_App_ca_info_enq(AM_CI_Handle_t handle);
/**\brief ci enquie ai
 * \param [in] handle ci opened handle
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_CI_App_ai_enquiry(AM_CI_Handle_t handle);

/*user need to free the generated capmt with free() in the end */
/**\brief ci generate ca pmt
 * \param [in] pmt CA modue recivered pmt
 * \param [in] pmt_size pmt length
 * \param [out] capmt generated ca pmt
 * \param [out] capmt_size capmt length
 * \param [in] ca_list_management see AM_CI_CA_LIST_MANAGEMENT
 * \param [in] ca_pmt_cmd_id see AM_CI_CA_PMT_CMD_ID
 * \param [in] moveca Whether to delete ca descriptors
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_CI_GenerateCAPMT(unsigned char *pmt, unsigned int pmt_size,
											unsigned char **capmt, unsigned int *capmt_size,
											int ca_list_management, int ca_pmt_cmd_id,
											int moveca);
/**\brief ci match ca by caid
 * \param [in] handle ci opened handle
 * \param [in] caid CA id
 * \param [out] match 1:match 0: unmatch.
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_CI_MatchCAID(AM_CI_Handle_t handle, unsigned int caid, int *match);
/**\brief ci get ca by handle
 * \param [in] handle ci opened handle
 * \param [out] ca AM_CA_t pointer of pointer
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_CI_CAMAN_getCA(AM_CI_Handle_t handle, AM_CA_t **ca);



#ifdef __cplusplus
}
#endif

#endif/*_AM_CI_H_*/
