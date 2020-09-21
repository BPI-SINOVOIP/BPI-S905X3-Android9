/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief CI data used in CA manager
 *
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-08-06: create the document
 ***************************************************************************/
#ifndef _CA_CI_H_
#define _CA_CI_H_

#include "am_caman.h"
#include "am_ci.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ca_ci_msg_s ca_ci_msg_t;


typedef struct ca_ci_appinfo_s ca_ci_appinfo_t;
typedef struct ca_ci_cainfo_s ca_ci_cainfo_t;

typedef struct ca_ci_mmi_close_s ca_ci_mmi_close_t;
typedef struct ca_ci_mmi_display_control_s ca_ci_mmi_display_control_t;
typedef struct ca_ci_mmi_enq_s ca_ci_mmi_enq_t;

typedef struct ca_ci_answer_enq_s ca_ci_answer_enq_t;
typedef struct ca_ci_answer_menu_s ca_ci_answer_menu_t;
typedef struct ca_ci_close_mmi_s ca_ci_close_mmi_t;
typedef struct ca_ci_set_pmt_s ca_ci_set_pmt_t;

/**\brief message type of ca ci module,app and ca send messages to each other by this message type */
enum ca_ci_msg_type
{
	/*msgs from ca to app*/
	ca_ci_msg_type_appinfo,								/**< send app info message from ca to app*/
	ca_ci_msg_type_cainfo,								/**< send ca info message from ca to app*/
	ca_ci_msg_type_mmi_close,							/**< send mmi close message from ca to app*/
	ca_ci_msg_type_mmi_display_control,		/**< send mmi displayer message from ca to app*/
	ca_ci_msg_type_mmi_enq,								/**< send mmi enquie message from ca to app*/
	ca_ci_msg_type_mmi_menu,							/**< send mmi meun message from ca to app*/
	ca_ci_msg_type_mmi_list,							/**< send mmi list message from ca to app*/

	/*msgs from app to ca*/
	ca_ci_msg_type_enter_menu,						/**< send enter meun message from app to ca*/
	ca_ci_msg_type_answer,								/**< send answer message from app to ca*/
	ca_ci_msg_type_answer_menu,						/**< send answer menu message from app to ca*/
	ca_ci_msg_type_close_mmi,							/**< send close mmi message from app to ca*/
	ca_ci_msg_type_set_pmt,								/**< send set pmt message from app to ca*/
	ca_ci_msg_type_get_cainfo,						/**< send get ca info message from app to ca*/
	ca_ci_msg_type_get_appinfo,						/**< send get app info message from app to ca*/
};

/**\brief app info of ci*/
struct ca_ci_appinfo_s {
	uint8_t application_type;						/**< application type*/
	uint16_t application_manufacturer;	/**< application manufacturer*/
	uint16_t manufacturer_code;					/**< manufacturer code*/
	uint8_t menu_string_length;					/**< menu string length*/
	uint8_t menu_string[0];							/**< menu string*/
};
/**\brief ca info of ci,used to notify app get ca info*/
struct ca_ci_cainfo_s {
	uint32_t ca_id_count;		/**< ca id count*/
	uint16_t ca_ids[0];			/**< ca_ids ca id array*/
};
/**\brief mmi close info of ci,used to notify app close mmi menu*/
struct ca_ci_mmi_close_s {
	uint8_t cmd_id;	/**< cmd_id	see AM_CI_MMI_CLOSE_MMI_CMD_ID*/
	uint8_t delay;	/**< delay time*/
};
/**\brief mmi display info of ci,used to notify app mmi display info*/
struct ca_ci_mmi_display_control_s {
	uint8_t cmd_id; 		/**< cmd_id	see MMI_DISPLAY_CONTROL_CMD_ID* in en50221_app_mmi.h */
	uint8_t mmi_mode;		/**< see MMI_MODE_* in en50221_app_mmi.h*/
};
/**\brief mmi enquie info of ci,used to notify app to show mmi eng menu*/
struct ca_ci_mmi_enq_s {
	uint8_t blind_answer;						/**< set to 1 menus that user input has not to be displayed*/
	uint8_t expected_answer_length;	/**< expected length,if set to FF means unkown*/
	uint32_t text_size;							/**< input text string length*/
	uint8_t text[0];								/**< input text string*/
};

/*parse as this struct definition
 !!byte by byte, not aligned!!
struct ca_ci_mmi_text_s {
	unsigned int text_size;
	unsigned char text[0];
};

struct ca_ci_mmi_menu_list_s {
	ca_ci_mmi_text_t title;
	ca_ci_mmi_text_t sub_title;
	ca_ci_mmi_text_t bottom;
	uint32_t item_count;
	ca_ci_mmi_text_t items[0];
	uint32_t item_raw_length;
	uint8_t items_raw[0];
};
*/
/**\brief mmi answer enquie info of ci,uesd to answer or cancel ca_ci_msg_type_mmi_enq type message*/
struct ca_ci_answer_enq_s {
	int answer_id;		/**< set 1 means answer,set 0 means cancel*/
	int size;					/**< answer text string length*/
	char answer[0];		/**< answer text string*/
};
/**\brief mmi answer menu info of ci,used to select menu item*/
struct ca_ci_answer_menu_s {
	int select;			/**< select menu item index,set 0 means return parent menu*/
};
/**\brief mmi close info of ci,used to close mmi*/
struct ca_ci_close_mmi_s {
	int cmd_id;			/**< mmi close cmd id,see AM_CI_MMI_CLOSE_MMI_CMD_ID*/
	int delay;			/**< delay time*/
};
/**\brief set pmt info of ci,used send pmt to cam card*/
struct ca_ci_set_pmt_s {
	unsigned int list_management;	/**< see AM_CI_CA_LIST_MANAGEMENT*/
	unsigned int cmd_id;					/**< see AM_CI_CA_PMT_CMD_ID*/
	unsigned int length;					/**< pmt length*/
	unsigned char pmt[0];					/**< received pmt*/
};

/**\brief message type of ci
 * msg ： this member will store message connent
 * for example :
 *			ca_ci_appinfo_s
 *			ca_ci_cainfo_s
 *			ca_ci_mmi_close_s
 *			ca_ci_mmi_display_control_s
 *			ca_ci_mmi_enq_s
 *			ca_ci_answer_enq_s
 *			ca_ci_answer_menu_s
 *			ca_ci_close_mmi_s
 *			ca_ci_set_pmt_s
 */
struct ca_ci_msg_s
{
	int type;								/**< message type,see ca_ci_msg_type*/
	unsigned char msg[0];		/**< message connent obj*/
};

extern AM_CA_t ca_ci;

#ifdef __cplusplus
}
#endif

#endif

