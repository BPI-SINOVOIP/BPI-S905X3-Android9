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
/**\file
 * \brief GUI测试程序
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-11-04: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <am_gui.h>
#include <am_inp.h>
#include <am_osd.h>
#include <am_font.h>
#include <string.h>

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define OSD_DEV_NO 0
#define INP_DEV_NO 0

#define OSD_FORMAT AM_OSD_FMT_COLOR_8888
#define OSD_WIDTH  720
#define OSD_HEIGHT 576
#define OSD_OUTPUT_WIDTH  720
#define OSD_OUTPUT_HEIGHT 576

#define FONT_PATH  "/usr/share/fonts/truetype/wqy/wqy-microhei.ttc"
//#define FONT_PATH  "/usr/share/fonts/wenquanyi/wqy-zenhei/wqy-zenhei.ttc"


/****************************************************************************
 * Functions
 ***************************************************************************/

static AM_ErrorCode_t btn_evt_cb(AM_GUI_Widget_t *widget, AM_GUI_WidgetEvent_t *evt)
{
	switch(evt->type)
	{
		case AM_GUI_WIDGET_EVT_PRESSED:
			printf("pressed button %p\n", widget);
		return AM_SUCCESS;
		default:
		break;
	}
	
	return AM_GUI_ERR_UNKNOWN_EVENT;
}

int main(int argc, char **argv)
{
	AM_OSD_OpenPara_t osd_p;
	AM_INP_OpenPara_t inp_p;
	AM_GUI_CreatePara_t gui_p;
	AM_FONT_Attr_t attr;
	AM_GUI_t *gui;
	
	memset(&osd_p, 0, sizeof(osd_p));
	osd_p.format = OSD_FORMAT;
	osd_p.width  = OSD_WIDTH;
	osd_p.height = OSD_HEIGHT;
	osd_p.output_width  = OSD_OUTPUT_WIDTH;
	osd_p.output_height = OSD_OUTPUT_HEIGHT;
	osd_p.enable_double_buffer = AM_TRUE;
	AM_OSD_Open(OSD_DEV_NO, &osd_p);
	
	memset(&inp_p, 0, sizeof(inp_p));
	AM_INP_Open(INP_DEV_NO, &inp_p);
	
	AM_FONT_Init();
	
	attr.name   = "default";
	attr.width  = 0;
	attr.height = 0;
	attr.encoding = AM_FONT_ENC_UTF16;
	attr.charset = AM_FONT_CHARSET_UNICODE;
	attr.flags  = 0;
	
	if(AM_FONT_LoadFile(FONT_PATH, "freetype", &attr)!=AM_SUCCESS)
	{
		goto end;
	}
	
	memset(&gui_p, 0, sizeof(gui_p));
	gui_p.inp_dev_no = INP_DEV_NO;
	gui_p.osd_dev_no = OSD_DEV_NO;
	AM_GUI_Create(&gui_p, &gui);
	
	{
		AM_GUI_Label_t *lab;
		AM_GUI_Button_t *btn;
		AM_GUI_Window_t *win;
		AM_GUI_Box_t *vbox, *hbox;
		AM_GUI_Table_t *tab;
		AM_GUI_Progress_t *prog;
		AM_GUI_Scroll_t *scroll;
		
		AM_GUI_WindowCreate(gui, &win);
		AM_GUI_WidgetAppendChild(NULL, AM_GUI_WIDGET(win));
		AM_GUI_WindowSetCaption(win, "Win1");
		AM_GUI_WidgetShow(AM_GUI_WIDGET(win));
		
		AM_GUI_BoxCreate(gui, AM_GUI_DIR_HORIZONTAL, &hbox);
		AM_GUI_WidgetAppendChild(AM_GUI_WIDGET(win), AM_GUI_WIDGET(hbox));
		AM_GUI_WidgetShow(AM_GUI_WIDGET(hbox));
		
		AM_GUI_ProgressCreate(gui, AM_GUI_DIR_VERTICAL, &prog);
		AM_GUI_ProgressSetValue(prog, 50);
		AM_GUI_BoxAppend(hbox, AM_GUI_WIDGET(prog), 0, 0);
		AM_GUI_WidgetShow(AM_GUI_WIDGET(prog));
		
		AM_GUI_ScrollCreate(gui, AM_GUI_DIR_VERTICAL, &scroll);
		AM_GUI_ScrollSetValue(scroll, 66);
		AM_GUI_BoxAppend(hbox, AM_GUI_WIDGET(scroll), 0, 0);
		AM_GUI_WidgetShow(AM_GUI_WIDGET(scroll));
		
		AM_GUI_BoxCreate(gui, AM_GUI_DIR_VERTICAL, &vbox);
		AM_GUI_BoxAppend(hbox, AM_GUI_WIDGET(vbox), AM_GUI_BOX_FL_EXPAND, 0);
		AM_GUI_WidgetShow(AM_GUI_WIDGET(vbox));
		
		AM_GUI_TextButtonCreate(gui, "Button I", &btn);
		AM_GUI_BoxAppend(hbox, AM_GUI_WIDGET(btn), AM_GUI_BOX_FL_EXPAND, 0);
		AM_GUI_WidgetSetUserCallback(AM_GUI_WIDGET(btn), btn_evt_cb);
		AM_GUI_WidgetShow(AM_GUI_WIDGET(btn));
		
		AM_GUI_TextButtonCreate(gui, "Button a", &btn);
		AM_GUI_BoxAppend(vbox, AM_GUI_WIDGET(btn), AM_GUI_BOX_FL_EXPAND, 0);
		AM_GUI_WidgetSetUserCallback(AM_GUI_WIDGET(btn), btn_evt_cb);
		AM_GUI_WidgetShow(AM_GUI_WIDGET(btn));
		
		AM_GUI_TextButtonCreate(gui, "Button b", &btn);
		AM_GUI_BoxAppend(vbox, AM_GUI_WIDGET(btn), AM_GUI_BOX_FL_EXPAND, 0);
		AM_GUI_WidgetSetUserCallback(AM_GUI_WIDGET(btn), btn_evt_cb);
		AM_GUI_WidgetShow(AM_GUI_WIDGET(btn));
		
		AM_GUI_TextButtonCreate(gui, "Button c", &btn);
		AM_GUI_BoxAppend(vbox, AM_GUI_WIDGET(btn), AM_GUI_BOX_FL_EXPAND, 0);
		AM_GUI_WidgetSetUserCallback(AM_GUI_WIDGET(btn), btn_evt_cb);
		AM_GUI_WidgetShow(AM_GUI_WIDGET(btn));
		
		AM_GUI_TableCreate(gui, &tab);
		AM_GUI_BoxAppend(vbox, AM_GUI_WIDGET(tab), AM_GUI_BOX_FL_EXPAND, 0);
		AM_GUI_WidgetShow(AM_GUI_WIDGET(tab));
		
		AM_GUI_TableAppendRow(tab, 0, 0);
		
		AM_GUI_TextButtonCreate(gui, "Button1", &btn);
		AM_GUI_TableAppendCell(tab, AM_GUI_WIDGET(btn), 0, 0, 0, 1, 1);
		AM_GUI_WidgetSetUserCallback(AM_GUI_WIDGET(btn), btn_evt_cb);
		AM_GUI_WidgetShow(AM_GUI_WIDGET(btn));
		
		AM_GUI_TextButtonCreate(gui, "Button2", &btn);
		AM_GUI_TableAppendCell(tab, AM_GUI_WIDGET(btn), 0, 0, 1, 1, 1);
		AM_GUI_WidgetSetUserCallback(AM_GUI_WIDGET(btn), btn_evt_cb);
		AM_GUI_WidgetShow(AM_GUI_WIDGET(btn));
		
		AM_GUI_TextButtonCreate(gui, "Button3", &btn);
		AM_GUI_TableAppendCell(tab, AM_GUI_WIDGET(btn), 0, 0, 2, 1, 1);
		AM_GUI_WidgetSetUserCallback(AM_GUI_WIDGET(btn), btn_evt_cb);
		AM_GUI_WidgetShow(AM_GUI_WIDGET(btn));
		
		AM_GUI_TableAppendRow(tab, 0, 0);
		
		AM_GUI_TextButtonCreate(gui, "Button4", &btn);
		AM_GUI_TableAppendCell(tab, AM_GUI_WIDGET(btn), 0, 0, 0, 1, 1);
		AM_GUI_WidgetSetUserCallback(AM_GUI_WIDGET(btn), btn_evt_cb);
		AM_GUI_WidgetShow(AM_GUI_WIDGET(btn));

		AM_GUI_TextButtonCreate(gui, "Button5", &btn);
		AM_GUI_TableAppendCell(tab, AM_GUI_WIDGET(btn), 0, 0, 1, 1, 1);
		AM_GUI_WidgetSetUserCallback(AM_GUI_WIDGET(btn), btn_evt_cb);
		AM_GUI_WidgetShow(AM_GUI_WIDGET(btn));
		
		AM_GUI_TextButtonCreate(gui, "Button6", &btn);
		AM_GUI_TableAppendCell(tab, AM_GUI_WIDGET(btn), 0, 0, 2, 1, 1);
		AM_GUI_WidgetSetUserCallback(AM_GUI_WIDGET(btn), btn_evt_cb);
		AM_GUI_WidgetShow(AM_GUI_WIDGET(btn));
		
		AM_GUI_TableAppendRow(tab, 0, 0);
		
		AM_GUI_TextButtonCreate(gui, "Button7", &btn);
		AM_GUI_TableAppendCell(tab, AM_GUI_WIDGET(btn), 0, 0, 0, 1, 1);
		AM_GUI_WidgetSetUserCallback(AM_GUI_WIDGET(btn), btn_evt_cb);
		AM_GUI_WidgetShow(AM_GUI_WIDGET(btn));
		
		AM_GUI_TextButtonCreate(gui, "Button8", &btn);
		AM_GUI_TableAppendCell(tab, AM_GUI_WIDGET(btn), 0, 0, 1, 1, 1);
		AM_GUI_WidgetSetUserCallback(AM_GUI_WIDGET(btn), btn_evt_cb);
		AM_GUI_WidgetShow(AM_GUI_WIDGET(btn));
		
		AM_GUI_TextButtonCreate(gui, "Button9", &btn);
		AM_GUI_TableAppendCell(tab, AM_GUI_WIDGET(btn), 0, 0, 2, 1, 1);
		AM_GUI_WidgetSetUserCallback(AM_GUI_WIDGET(btn), btn_evt_cb);
		AM_GUI_WidgetShow(AM_GUI_WIDGET(btn));
		
		AM_GUI_TextLabelCreate(gui, "Label", &lab);
		AM_GUI_LabelSetMultiline(lab, AM_TRUE);
		AM_GUI_BoxAppend(hbox, AM_GUI_WIDGET(lab), AM_GUI_BOX_FL_EXPAND, 0);
		AM_GUI_WidgetShow(AM_GUI_WIDGET(lab));
		
		AM_GUI_WidgetLayOut(AM_GUI_WIDGET(win));
	}
	
	AM_GUI_Main(gui);
end:
	AM_GUI_Destroy(gui);
	AM_FONT_Quit();
	AM_OSD_Close(OSD_DEV_NO);
	AM_INP_Close(INP_DEV_NO);
	return 0;
}

