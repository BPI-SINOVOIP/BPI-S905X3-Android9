#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file am_ci_test.c
 * \brief ci模块测试程序
 *
 * \author chen hua ling <hualing.chen@amlogic.com>
 * \date 2016-08-23: create the document
 ***************************************************************************/

#define printf_LEVEL 1
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <am_fend.h>
#include <am_fend_ctrl.h>
#include <am_dmx.h>
#include <am_db.h>
#include <am_scan.h>
#include <am_epg.h>
#include <am_debug.h>

#include "am_caman.h"
#include "am_ci.h"
#include "ca_ci.h"

/****************************************************************************
 * Macro definitions
 ***************************************************************************/
#define FEND_DEV_NO 0
#define AV_DEV_NO 0
#define DMX_DEV_NO 0

#define MAX_PROG_CNT 300
#define MAX_NAME_LEN 64

/****************************************************************************
 * Type definitions
 ***************************************************************************/


enum
{
	MENU_TYPE_MENU,
	MENU_TYPE_APP
};


typedef struct
{
	const int	id;
	const int	parent;
	const char	*title;
	const char	*cmd;
	const int	type;
	void (*app)(void);
	void (*leave)(void);
	void (*input)(const char *cmd);
}CommandMenu;

static CommandMenu *curr_menu;
static AM_Bool_t go = AM_TRUE;

/****************************************************************************
 * static Functions
 ***************************************************************************/
static void show_main_menu(void);

#define MMI_STATE_CLOSED 0
#define MMI_STATE_OPEN 1
#define MMI_STATE_ENQ 2
#define MMI_STATE_MENU 3

static int mmi_state = MMI_STATE_CLOSED;
static void hex_dump(void *data, int size)
{
    /* dumps size bytes of *data to stdout. Looks like:
     * [0000] 75 6E 6B 6E 6F 77 6E 20
     *                  30 FF 00 00 00 00 39 00 unknown 0.....9.
     * (in a single line of course)
     */

    unsigned char *p = data;
    unsigned char c;
    int n;
    char bytestr[4] = {0};
    char addrstr[10] = {0};
    char hexstr[ 16*3 + 5] = {0};
    char charstr[16*1 + 5] = {0};
    for (n=1; n <= size; n++) {
        if (n%16 == 1) {
            /* store address for this line */
            snprintf(addrstr, sizeof(addrstr), "%.4x",
               ((unsigned int)p-(unsigned int)data) );
        }
        c = *p;
        if (isalnum(c) == 0) {
            c = '.';
        }

        /* store hex str (for left side) */
        snprintf(bytestr, sizeof(bytestr), "%02X ", *p);
        strncat(hexstr, bytestr, sizeof(hexstr)-strlen(hexstr)-1);

        /* store char str (for right side) */
        snprintf(bytestr, sizeof(bytestr), "%c", c);
        strncat(charstr, bytestr, sizeof(charstr)-strlen(charstr)-1);

        if (n%16 == 0) {
            /* line completed */
            printf("[%4.4s]   %-50.50s  %s\n", addrstr, hexstr, charstr);
            hexstr[0] = 0;
            charstr[0] = 0;
        } else if(n%8 == 0) {
            /* half line: add whitespaces */
            strncat(hexstr, "  ", sizeof(hexstr)-strlen(hexstr)-1);
            strncat(charstr, " ", sizeof(charstr)-strlen(charstr)-1);
        }
        p++; /* next byte */
    }

    if (strlen(hexstr) > 0) {
        /* print rest of buffer if not empty */
        printf("[%4.4s]   %-50.50s  %s\n", addrstr, hexstr, charstr);
    }
}

static int caman_cb(char *name, AM_CA_Msg_t *msg)
{
	AM_ErrorCode_t err;

	if (!msg)
	{
		AM_DEBUG(1, "\t\t-->|APP ??? msg (NULL) ???");
		return -1;
	}

	if (msg->dst != AM_CA_MSG_DST_APP)
	{
		AM_DEBUG(1, "\t\t-->|APP ??? msg dst is not app(%d) ???", msg->dst);
		return -1;
	}

	if (0 == strcmp("ci0", name))
	{
		ca_ci_msg_t *cimsg = (ca_ci_msg_t *)msg->data;
		//hex_dump(msg->data, msg->len);
		switch (cimsg->type)
		{
			#define PIN(i, x) AM_DEBUG(1,"\t\t-->|"# i"("# x"):\t0x%x", x)
			#define PIS(i, x, l) AM_DEBUG(1,"\t\t-->|"# i"("# x"):\t%.*s", l, x)
			#define PInS(i, n, x, l) AM_DEBUG(1,"\t\t-->|"# i":\t(%d):%.*s", n, l, x)
			#define PI_S(i, x, l) AM_DEBUG(1,"\t\t-->|"# i":\t%.*s", l, x)

			case ca_ci_msg_type_appinfo:
				{
					ca_ci_appinfo_t *appinfo = (ca_ci_appinfo_t*)cimsg->msg;
					printf("ca appinfo-----\r\n");
					#define PN(x) PIN(APPINFO, x)
					#define PS(x, n) PIS(APPINFO, x, n)

					PN(appinfo->application_type);
					PN(appinfo->application_manufacturer);
					PN(appinfo->manufacturer_code);
					PS(appinfo->menu_string, appinfo->menu_string_length);

					#undef PN
					#undef PS
				}
				break;
			case ca_ci_msg_type_cainfo:
				{
					int i;
					printf("ca info-----\r\n");
					ca_ci_cainfo_t *cainfo = (ca_ci_cainfo_t*)cimsg->msg;

					for (i=0; i<cainfo->ca_id_count; i++) {
						PIN(CAINFO, i);
						PIN(CAINFO, cainfo->ca_ids[i]);
					}
				}
				break;

			case ca_ci_msg_type_mmi_close:
				printf("close ca -----\r\n");
				mmi_state = MMI_STATE_CLOSED;
				break;

			case ca_ci_msg_type_mmi_display_control:
				mmi_state = MMI_STATE_OPEN;
				printf("open ca -----\r\n");
				break;

			case ca_ci_msg_type_mmi_enq:
				{
					ca_ci_mmi_enq_t *enq = (ca_ci_mmi_enq_t*)cimsg->msg;
					printf("ca mmi enq-----\r\n");
					PIN(ENQ, enq->blind_answer);
					PIN(ENQ, enq->expected_answer_length);
					PIS(ENQ, enq->text, enq->text_size);

					mmi_state = MMI_STATE_ENQ;
				}
				break;

			case ca_ci_msg_type_mmi_menu:
			case ca_ci_msg_type_mmi_list:
				{
					unsigned char *p = cimsg->msg;
					int l; int i, s;
					#define _getint(p) (p[0] | (p[1]<<8) | (p[2]<<16) | (p[3]<<24))
					l=_getint(p);
					PI_S(MENU:titile, &p[4], l); p+=(l+4);
					l=_getint(p);
					PI_S(MENU:subtitile, &p[4], l); p+=(l+4);
					l=_getint(p);
					PI_S(MENU:bottom, &p[4], l); p+=(l+4);
					s=_getint(p); p+=4;
					for (i=0; i<s; i++)
					{
						l = _getint(p);
						PInS(MENU:item, i+1, &p[4], l); p+=(l+4);
					}
					printf("ca mmi menus-----\r\n");
					mmi_state = MMI_STATE_MENU;
				}
				break;

			default:
				AM_DEBUG(1, "\t\t-->|APP ??? unkown ci msg type(%d) ??? ", cimsg->type);
				break;

			#undef PIN
			#undef PIS
		}

		if ((err = AM_CAMAN_freeMsg(msg)) != AM_SUCCESS)
		{
			AM_DEBUG(1, "\t\t-->|APP ??? free msg fail[%x]", err);
		}
	}
	else
	{
		AM_DEBUG(1, "\t\t-->|??? ca(%s) ???", name);
		return -1;
	}

	return 0;
}

static void run_ci(void)
{
	AM_CA_Msg_t msg;
	ca_ci_msg_t cimsg;
	printf("into run ci--\r\n");
	cimsg.type = ca_ci_msg_type_enter_menu;
	msg.type = 0;
	msg.dst = AM_CA_MSG_DST_CA;
	msg.data = (unsigned char*)&cimsg;
	msg.len = sizeof(cimsg);
	AM_CAMAN_putMsg("ci0", &msg);
}

static void leave_ci(void)
{
	AM_CA_Msg_t msg;
	int lmsg = sizeof(ca_ci_msg_t)+sizeof(ca_ci_close_mmi_t);
	ca_ci_msg_t *cimsg = malloc(lmsg);
	assert(cimsg);

	ca_ci_close_mmi_t *close_mmi = (ca_ci_close_mmi_t *)cimsg->msg;

	close_mmi->cmd_id = AM_CI_MMI_CLOSE_MMI_CMD_ID_IMMEDIATE;
	close_mmi->delay = 0;
	cimsg->type = ca_ci_msg_type_close_mmi;
	msg.type = 0;
	msg.dst = AM_CA_MSG_DST_CA;
	msg.data = (unsigned char*)cimsg;
	msg.len = lmsg;
	AM_CAMAN_putMsg("ci0", &msg);

	free(cimsg);
}

static void ci_input(const char *cmd)
{
	AM_CA_Msg_t msg;
	ca_ci_msg_t *cimsg;
	int lmsg;
	printf("ci input----\r\n");
	if (0 == strcmp(cmd, "u"))
	{
		lmsg = sizeof(ca_ci_msg_t)+sizeof(ca_ci_answer_enq_t);
		cimsg = malloc(lmsg);
		assert(cimsg);

		ca_ci_answer_enq_t *aenq = (ca_ci_answer_enq_t *)cimsg->msg;

		aenq->answer_id = AM_CI_MMI_ANSW_CANCEL;
		aenq->size = 0;
		cimsg->type = ca_ci_msg_type_answer;
		msg.type = 0;
		msg.dst = AM_CA_MSG_DST_CA;
		msg.data = (unsigned char*)cimsg;
		msg.len = lmsg;
		AM_CAMAN_putMsg("ci0", &msg);

		free(cimsg);

		return;
	}

	switch (mmi_state) {
		case MMI_STATE_CLOSED:
		case MMI_STATE_OPEN:
			break;

		case MMI_STATE_ENQ:
			{
				int len = strlen(cmd);
				lmsg = sizeof(ca_ci_msg_t) + sizeof(ca_ci_answer_enq_t) + len;
				cimsg = malloc(lmsg);
				assert(cimsg);

				ca_ci_answer_enq_t *aenq = (ca_ci_answer_enq_t *)cimsg->msg;

				if (!len) {
					aenq->answer_id = AM_CI_MMI_ANSW_CANCEL;
					aenq->size = 0;
				} else {
					aenq->answer_id = AM_CI_MMI_ANSW_ANSWER;
					aenq->size = len;
					memcpy(aenq->answer, cmd, len);
				}
				cimsg->type = ca_ci_msg_type_answer;
				msg.type = 0;
				msg.dst = AM_CA_MSG_DST_CA;
				msg.data = (unsigned char*)cimsg;
				msg.len = lmsg;
				AM_CAMAN_putMsg("ci0", &msg);

				free(cimsg);
			}

			mmi_state = MMI_STATE_OPEN;
			break;

		case MMI_STATE_MENU:
			{
				lmsg = sizeof(ca_ci_msg_t)+sizeof(ca_ci_answer_menu_t);
				cimsg = malloc(lmsg);
				assert(cimsg);

				ca_ci_answer_menu_t *amenu = (ca_ci_answer_menu_t *)cimsg->msg;

				amenu->select = atoi(cmd);

				cimsg->type = ca_ci_msg_type_answer_menu;
				msg.type=0;
				msg.dst = AM_CA_MSG_DST_CA;
				msg.data = (unsigned char*)cimsg;
				msg.len = lmsg;
				AM_CAMAN_putMsg("ci0", &msg);

				free(cimsg);
			}
			mmi_state = MMI_STATE_OPEN;
			break;
	}
}


static void run_quit(void)
{
	go = AM_FALSE;
}


static CommandMenu menus[]=
{
	{0, -1, "Main Menu", 		NULL,	MENU_TYPE_MENU,	NULL,				NULL, 		NULL},
	{1, 0, "1. Quit",			"1",	MENU_TYPE_APP,	run_quit,			NULL,		NULL},
	{2, 0, "2. CI",			"2",	MENU_TYPE_APP,	run_ci,			leave_ci,		ci_input},
	{3, 0, "y. start send pmt",			"y",	MENU_TYPE_APP,	NULL,			NULL,		NULL},
	{4, 0, "n. stop send pmt",			"n",	MENU_TYPE_APP,	NULL,			NULL,		NULL},
};

static void display_menu(void)
{
	int i;

	assert(curr_menu);

	printf("-------------------------------------------\n");
	printf("\t\t%s\n", curr_menu->title);
	printf("-------------------------------------------\n");
	for (i=0; i<AM_ARRAY_SIZE(menus); i++)
	{
		if (menus[i].parent == curr_menu->id)
			printf("\t\t%s\n", menus[i].title);
	}
	printf("-------------------------------------------\n");
}


static void show_main_menu(void)
{
	curr_menu = &menus[0];
	display_menu();
}

static int menu_input(const char *cmd)
{
	int i;
	char buf[256];
	assert(curr_menu);

	if (!strcmp(cmd, "b"))
	{
		if (curr_menu->leave)
			curr_menu->leave();
		/*退回上级菜单*/
		if (curr_menu->parent != -1)
		{
			curr_menu = &menus[curr_menu->parent];
			if (curr_menu->type == MENU_TYPE_MENU)
			{
				display_menu();
			}
			else if (curr_menu->app)
			{
				curr_menu->app();
			}
		}
		return 0;
	}
	else if (!strcmp(cmd, "y"))
	{
		printf("---please input service id:\n");
		gets(buf);
		int serviceid = atoi(buf);
		AM_CAMAN_startService(serviceid, "ci0");
		return 0;
	}
	else if (!strcmp(cmd, "n"))
	{
		printf("----please input service id:\n");
		gets(buf);
		int serviceid = atoi(buf);
		AM_CAMAN_stopService(serviceid);
		return 0;
	}
/*从预定义中查找按键处理*/
	for (i=0; i<AM_ARRAY_SIZE(menus); i++)
	{
		printf("%s:::%s id=%d parent=%d\r\n", cmd,menus[i].cmd,curr_menu->id,menus[i].parent);
		if (menus[i].parent == curr_menu->id && !strcmp(cmd, menus[i].cmd))
		{
			curr_menu = &menus[i];
			if (menus[i].type == MENU_TYPE_MENU)
			{
				printf("display_menu--\r\n");
				display_menu();
			}
			else if (menus[i].app)
			{
				printf("app show----%d-\r\n",i);
				menus[i].app();
			}
			return 0;
		}
	}
    printf("return -1 cmd:%s\n",cmd);
	return -1;
}

//#define TEST_CA_DUMMY
//#define TEST_CA_CI

static int start_tv_test()
{
	char buf[256];
	AM_CI_OpenPara_t  ci_para;
	static AM_CI_Handle_t ci;
	AM_CA_t *ca;

	AM_CAMAN_OpenParam_t caman_para;
	AM_CA_Opt_t ca_opt;
	memset(&caman_para, 0, sizeof(caman_para));
	caman_para.ts.dmx_fd = DMX_DEV_NO;
	caman_para.ts.fend_fd = FEND_DEV_NO;
	AM_TRY(AM_CAMAN_Open(&caman_para));

	memset(&ci_para, 0, sizeof(ci_para));
	AM_CI_Open(0, 0, &ci_para, &ci);
//	AM_TRY(AM_CI_Start(ci));

	/*register the ci*/
	AM_CI_CAMAN_getCA(ci, &ca);
	memset(&ca_opt, 0, sizeof(ca_opt));
	ca_opt.auto_disable = 0;
	AM_CAMAN_registerCA("ci0", ca, &ca_opt);
	AM_CAMAN_setCallback("ci0", caman_cb);
	AM_CAMAN_openCA("ci0");

	show_main_menu();
	int auto_test = 0;
	int cmd = 2;
	if (auto_test == 1) {
		sleep(10);
		sprintf(buf, "%d", cmd);
		if (menu_input(buf) < 0)
		{
			/*交由应用处理*/
			if (curr_menu->input)
			{
				curr_menu->input(buf);
			}
		}
		sleep(5);
	}

	while (go)
	{
		if (auto_test == 1 || (gets(buf) && curr_menu))
		{
			if (menu_input(buf) < 0)
			{
				/*交由应用处理*/
				if (curr_menu->input)
				{
					curr_menu->input(buf);
				}
			}
			if (auto_test == 1) {
				if (cmd ==2 ) {
					cmd = 0;
					sprintf(buf,"%d",cmd);
				} else if (cmd ==0 ) {
					cmd = 2;
					sprintf(buf,"%d",cmd);
				}
				sleep(5);
			}
		}
	}
	printf("AM_CAMAN_unregisterCA\n");
	AM_CAMAN_unregisterCA("ci0");
	AM_CI_Close(ci);
	AM_CAMAN_Close();
	return 0;
}
int main(int argc, char **argv)
{
	AM_DMX_OpenPara_t para;
	// AM_FEND_OpenPara_t fpara;
	// memset(&fpara, 0, sizeof(fpara));
	// fpara.mode = FE_QPSK;
	// AM_TRY(AM_FEND_Open(FEND_DEV_NO, &fpara));
	memset(&para, 0, sizeof(para));
	AM_TRY(AM_DMX_Open(DMX_DEV_NO, &para));
	AM_DMX_SetSource(DMX_DEV_NO, AM_DMX_SRC_TS2);

	start_tv_test();

	AM_DMX_Close(DMX_DEV_NO);
	//AM_FEND_Close(FEND_DEV_NO);
	return 0;
}