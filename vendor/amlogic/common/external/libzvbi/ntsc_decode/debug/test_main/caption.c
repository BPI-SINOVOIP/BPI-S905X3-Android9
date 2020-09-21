/*
 *  libzvbi test
 *
 *  Copyright (C) 2000, 2001 Michael H. Schimek
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301, USA.
 */

/* $Id: caption.c,v 1.19 2008/03/01 07:37:20 mschimek Exp $ */

#undef NDEBUG

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#ifndef X_DISPLAY_MISSING

//#include <X11/Xlib.h>
//#include <X11/keysym.h>
//#include <X11/Xutil.h>

#include "vbi.h"
#include "exp-gfx.h"
#include "hamm.h"
#include "dvb_demux.h"
#include "sliced.h"
#include "sliced_vbi.h"
#define XDS_SEP_DEBUG(x) /* x */  x
vbi_decoder *		vbi;
vbi_pgno		pgno = -1;
vbi_dvb_demux *		dx;

/*
 *  Rudimentary render code for CC test.
 *  Attention: RGB 5:6:5 little endian only.
 */

#define DISP_WIDTH	640
#define DISP_HEIGHT	480

#define CELL_WIDTH	16
#define CELL_HEIGHT	26

//Display *		display;
int			screen;
//Colormap		cmap;
//Window			window;
//GC			gc;
//XEvent			event;
//XImage *		ximage;
uint32_t *		ximgdata;

int			shift = 0, step = 3;
int			sh_first, sh_last;

vbi_rgba		row_buffer[64 * CELL_WIDTH * CELL_HEIGHT];

#define COLORKEY 0x80FF80 /* where video looks through */

#define RGB565(rgba)							\
	(((((rgba) >> 16) & 0xF8) << 8) | ((((rgba) >> 8) & 0xFC) << 3)	\
	 | (((rgba) & 0xF8) >> 3))

static void
draw_video			(int			x0,
				 int			y0,
				 int			w,
				 int			h)
{
	uint32_t *canvas = ximgdata + x0 + y0 * DISP_WIDTH;
	int x, y;

	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++)
			canvas[x] = RGB565(COLORKEY);
		canvas += DISP_WIDTH;
	}
}

static void
draw_blank			(int			column,
				 int			width)
{
	vbi_rgba *canvas = row_buffer + column * CELL_WIDTH;
	int x, y;

	for (y = 0; y < CELL_HEIGHT; y++) {
		for (x = 0; x < CELL_WIDTH * width; x++)
			canvas[x] = COLORKEY;
		canvas += sizeof(row_buffer) / sizeof(row_buffer[0])
			/ CELL_HEIGHT;
	}
}

//* Not exactly efficient, but this is only a test 
static void
draw_row			(uint32_t *		canvas,
				 vbi_page *		pg,
				 int			row)
{
	int i, j, num_tspaces = 0;
	vbi_rgba *s = row_buffer;
	XDS_SEP_DEBUG(printf("draw_row = %d\n",row));
	for (i = 0; i < pg->columns; ++i) {	
		//XDS_SEP_DEBUG(printf("%02x  ", unicode_ccfont2 (pg->text[row * pg->columns + i].unicode, pg->text[row * pg->columns + i].->italic)));
		if (pg->text[row * pg->columns + i].opacity
		    == VBI_TRANSPARENT_SPACE) {
			num_tspaces++;
			continue;
		}

		if (num_tspaces > 0) {
			draw_blank(i - num_tspaces, num_tspaces);
			num_tspaces = 0; 
		}

		vbi_draw_cc_page_region (pg, VBI_PIXFMT_RGBA32_LE,
					 row_buffer + i * CELL_WIDTH,
					 sizeof(row_buffer) / CELL_HEIGHT,
					 i, row, 1, 1);
	}
	
	if (num_tspaces > 0)
		draw_blank(i - num_tspaces, num_tspaces);
		
	XDS_SEP_DEBUG(printf("\npg->columns * CELL_WIDTH= %d\n",pg->columns * CELL_WIDTH));
	return;
	for (i = 0; i < CELL_HEIGHT; i++) {
		for (j = 0; j < pg->columns * CELL_WIDTH; j++){
			canvas[j] = RGB565(s[j]);
		}
		s += sizeof(row_buffer) / sizeof(row_buffer[0]) / CELL_HEIGHT;
		
		canvas += DISP_WIDTH;
	}
}

static void
bump				(int			n,
				 vbi_bool		draw)
{
	uint32_t *canvas = ximgdata + 45 * DISP_WIDTH;
	XDS_SEP_DEBUG(printf("bump"));
	return;
	if (shift < n)
		n = shift;

	if (shift <= 0 || n <= 0)
		return;

	memmove (canvas + (sh_first * CELL_HEIGHT) * DISP_WIDTH,
		 canvas + (sh_first * CELL_HEIGHT + n) * DISP_WIDTH,
		 ((sh_last - sh_first + 1) * CELL_HEIGHT - n)
		 * DISP_WIDTH * 2);

	//if (draw)
	//	XPutImage (display, window, gc, ximage,
	//		   0, 0, 0, 0, DISP_WIDTH, DISP_HEIGHT);

	shift -= n;
}

static void
render				(vbi_page *		pg,
				 int			row)
{
	//* ushort *canvas = ximgdata + 48 + 45 * DISP_WIDTH; 
	XDS_SEP_DEBUG(printf("render row %d\n",row));
	if (shift > 0) {
		bump(shift, FALSE);
		draw_video (48, 45 + sh_last * CELL_HEIGHT,
			    DISP_WIDTH - 48, CELL_HEIGHT);
	}
	XDS_SEP_DEBUG(printf("render draw_row  %d\n",ximgdata + 48 + (45 + row * CELL_HEIGHT) * DISP_WIDTH));
	draw_row (ximgdata + 48 + (45 + row * CELL_HEIGHT) * DISP_WIDTH,
		  pg, row);

	//XPutImage (display, window, gc, ximage,
	//	   0, 0, 0, 0, DISP_WIDTH, DISP_HEIGHT);
}
/*
static void
clear				(vbi_page *		pg)
{
	pg = pg;

	draw_video (0, 0, DISP_WIDTH, DISP_HEIGHT);

	XPutImage (display, window, gc, ximage,
		   0, 0, 0, 0, DISP_WIDTH, DISP_HEIGHT);
}

static void
roll_up				(vbi_page *		pg,
				 int			first_row,
				 int			last_row)
{
	ushort scol, *canvas = ximgdata + 45 * DISP_WIDTH;
	vbi_rgba col;
	int i, j;

#if 1 //* soft 

	sh_first = first_row;
	sh_last = last_row;
	shift = 26;
	bump(step, FALSE);

	canvas += 48 + (((last_row * CELL_HEIGHT) + CELL_HEIGHT - step)
			* DISP_WIDTH);
	col = pg->color_map[pg->text[last_row * pg->columns].background];
	scol = RGB565 (col);

	for (j = 0; j < step; ++j) {
		if (pg->text[last_row * pg->columns].opacity
		    == VBI_TRANSPARENT_SPACE) {
			for (i = 0; i < CELL_WIDTH * pg->columns; ++i)
				canvas[i] = RGB565 (COLORKEY);
		} else {
			for (i = 0; i < CELL_WIDTH * pg->columns; ++i)
				canvas[i] = scol;
		}

		canvas += DISP_WIDTH;
	}

#else //* at once 

	memmove (canvas + first_row * CELL_HEIGHT * DISP_WIDTH,
		 canvas + (first_row + 1) * CELL_HEIGHT * DISP_WIDTH,
		 (last_row - first_row) * CELL_HEIGHT * DISP_WIDTH * 2);

	draw_video (48, 45 + last_row * CELL_HEIGHT,
		    DISP_WIDTH - 48, CELL_HEIGHT);

#endif

	XPutImage (display, window, gc, ximage,
		   0, 0, 0, 0, DISP_WIDTH, DISP_HEIGHT);
}

static void
xevent				(int			nap_usec);
*/
static void
cc_handler			(vbi_event *		ev,
				 void *			user_data)
{

	printf("cc_handler\n");
	vbi_page page;
	vbi_bool success;
	int row;

	user_data = user_data;

	if (pgno != -1 && ev->ev.caption.pgno != pgno)
		return;

	/* Fetching & rendering in the handler
           is a bad idea, but this is only a test */

	success = vbi_fetch_cc_page (vbi, &page, ev->ev.caption.pgno, TRUE);
	assert (success);
	XDS_SEP_DEBUG(printf("cc_handler***********success = %d\n",success));
#if 1 /* optional */
	if (abs (page.dirty.roll) > page.rows) {
		XDS_SEP_DEBUG(printf("clear"));
		//clear (&page);
	} else if (page.dirty.roll == -1) {
		XDS_SEP_DEBUG(printf("roll_up"));
		//roll_up (&page, page.dirty.y0, page.dirty.y1);
	} else {
#endif
		
		for (row = page.dirty.y0; row <= page.dirty.y1; ++row)
			render (&page, row);
	}

	vbi_unref_page (&page);
}









static void
xds_handler			(vbi_event *		ev,
				 void *			user_data)
{

	printf("xds_handler\n");
	vbi_page page;
	vbi_bool success;
	if(ev == NULL)
		return;
	
	
	vbi_program_info *	prog_info = ev->ev.prog_info;

	if(prog_info->rating_auth != VBI_RATING_AUTH_NONE)
	{
			printf("prog_info->rating_auth  = %d\n",prog_info->rating_auth );
	
			if(prog_info->rating_auth == VBI_RATING_AUTH_MPAA)
					XDS_SEP_DEBUG(printf("result*******************VBI_RATING_AUTH_MPAA\n"));
			if(prog_info->rating_auth == VBI_RATING_AUTH_TV_US)
					XDS_SEP_DEBUG(printf("result*******************VBI_RATING_AUTH_TV_US\n"));
			if(prog_info->rating_auth == VBI_RATING_AUTH_TV_CA_EN)
					XDS_SEP_DEBUG(printf("result*******************VBI_RATING_AUTH_TV_CA_EN\n"));
			if(prog_info->rating_auth == VBI_RATING_AUTH_TV_CA_FR)
					XDS_SEP_DEBUG(printf("result*******************VBI_RATING_AUTH_TV_CA_FR\n"));
			if(prog_info->rating_auth == VBI_RATING_AUTH_NONE)
					XDS_SEP_DEBUG(printf("result*******************VBI_RATING_AUTH_NONE\n"));
			
			if (prog_info->rating_dlsv == VBI_RATING_D)
				XDS_SEP_DEBUG(printf("result*******************VBI_RATING_D\n"));
			if (prog_info->rating_dlsv == VBI_RATING_L)
				XDS_SEP_DEBUG(printf("result*******************VBI_RATING_L\n"));
			if (prog_info->rating_dlsv == VBI_RATING_S)
				XDS_SEP_DEBUG(printf("result*******************VBI_RATING_S\n"));
			if (prog_info->rating_dlsv == VBI_RATING_V)
				XDS_SEP_DEBUG(printf("result*******************VBI_RATING_V\n"));
			XDS_SEP_DEBUG(printf("result*******************prog_info->rating_id = %d\n",prog_info->rating_id ));
			
	}
	
}



static void
reset				(void)
{
	vbi_page page;
	vbi_bool success;
	int row;

	success = vbi_fetch_cc_page (vbi, &page, pgno, TRUE);
	assert (success);

	for (row = 0; row <= page.rows; ++row)
		render (&page, row);

	vbi_unref_page (&page);
}

/*
 *  X11 stuff
 */
/*
static void
xevent				(int			nap_usec)
{
	while (XPending (display)) {
		XNextEvent (display, &event);

		switch (event.type) {
		case KeyPress:
		{
			int c = XLookupKeysym (&event.xkey, 0);

			switch (c) {
			case 'q':
			case 'c':
				exit (EXIT_SUCCESS);

			case '1' ... '8':
				pgno = c - '1' + 1;
				reset ();
				return;

			case XK_F1 ... XK_F8:
				pgno = c - XK_F1 + 1;
				reset ();
				return;
			}

			break;
		}

		case Expose:
			XPutImage (display, window, gc, ximage,
				   0, 0, 0, 0, DISP_WIDTH, DISP_HEIGHT);
			break;

		case ClientMessage:
			exit (EXIT_SUCCESS);
		}
	}

	bump (step, TRUE);

	usleep (nap_usec / 4);
}
*/
static vbi_bool
init_window			(int			ac,
				 char **		av)
{
	//Atom delete_window_atom;
	//XWindowAttributes wa;
	int i;

	ac = ac;
	av = av;

	//if (!(display = XOpenDisplay (NULL))) {
	//	return FALSE;
	//}

	//screen = DefaultScreen (display);
	//cmap = DefaultColormap (display, screen);
 
	//window = XCreateSimpleWindow (display,
	//			      RootWindow (display, screen),
	//			     0, 0,
	//			      DISP_WIDTH, DISP_HEIGHT,
	//			      2,
	//			       0xffffffff,
	//			       0x00000000);
	//if (!window) {
	//	return FALSE;
	//}

	//XGetWindowAttributes (display, window, &wa);
			
	//if (16 != wa.depth) {
	//	fprintf (stderr, "Can only run at "
	//		 "color depth 16 (5:6:5) LE\n");
	//	return FALSE;
	//}

	if (!(ximgdata = malloc (DISP_WIDTH * DISP_HEIGHT * 2))) {
		return FALSE;
	}

	for (i = 0; i < DISP_WIDTH * DISP_HEIGHT; ++i)
		ximgdata[i] = RGB565 (COLORKEY);

	//ximage = XCreateImage(display,
	//		      DefaultVisual (display, screen),
	//		      DefaultDepth (display, screen),
	//		      ZPixmap, 0, (char *) ximgdata,
	//		      DISP_WIDTH, DISP_HEIGHT,
	//		      8, 0);
	//if (!ximage) {
	//	return FALSE;
	//}

	//delete_window_atom = XInternAtom (display, "WM_DELETE_WINDOW", False);

	//XSelectInput (display, window,
	//	      KeyPressMask |
	//	      ExposureMask |
	//	      StructureNotifyMask);
	//XSetWMProtocols (display, window, &delete_window_atom, 1);
	//XStoreName (display, window, "Caption Test - [Q], [F1]..[F8]");

	//gc = XCreateGC (display, window, 0, NULL);

	//XMapWindow (display, window);
	       
	//XSync (display, False);

	//XPutImage (display, window, gc, ximage,
	//	   0, 0, 0, 0, DISP_WIDTH, DISP_HEIGHT);

	return TRUE;
}

static vbi_bool
decode_frame			(const vbi_sliced *	sliced,
				 unsigned int		n_lines,
				 const uint8_t *	raw,
				 const vbi_sampling_par *sp,
				 double			sample_time,
				 int64_t		stream_time)
{
	printf("decode_frame\n");
	raw = raw;
	sp = sp;
	stream_time = stream_time; /* unused */

	vbi_decode (vbi, sliced, n_lines, sample_time);

	/* xevent (1e6 / 30); */

	return TRUE;
}

/*
 *  Feed artificial caption
 */

static void
cmd				(unsigned int		n)
{
	vbi_sliced sliced;
	static double time = 0.0;

	sliced.id = VBI_SLICED_CAPTION_525;
	sliced.line = 21;
	sliced.data[0] = vbi_par8 (n >> 8);
	sliced.data[1] = vbi_par8 (n & 0x7F);

	vbi_decode (vbi, &sliced, 1, time);

	//xevent (33333);

	time += 1 / 29.97;
}

static void
printc				(int			c)
{
	cmd (c * 256 + 0x80);

	//xevent (33333);
}

static void
prints				(const char *		s)
{
	for (; s[0] && s[1]; s += 2)
		cmd (s[0] * 256 + s[1]);

	if (s[0])
		cmd (s[0] * 256 + 0x80);

	//xevent (33333);
}

enum {
	white, green, red, yellow, blue, cyan, magenta, black
};

static int
mapping_row [] = {
	2, 3, 4, 5,  10, 11, 12, 13, 14, 15,  0, 6, 7, 8, 9, -1
};

#define italic 7
#define underline 1
#define opaque 0
#define semi_transp 1

#define BACKG(bg, t)							\
	(cmd (0x2000), cmd (0x1020 + ((ch & 1) << 11) + (bg << 1) + t))
#define PREAMBLE(r, fg, u)						\
	cmd (0x1040 + ((ch & 1) << 11) + ((mapping_row[r] & 14) << 7)	\
	     + ((mapping_row[r] & 1) << 5) + (fg << 1) + u)
#define INDENT(r, fg, u)						\
	cmd (0x1050 + ((ch & 1) << 11) + ((mapping_row[r] & 14) << 7)	\
	     + ((mapping_row[r] & 1) << 5) + ((fg / 4) << 1) + u)
#define MIDROW(fg, u)	cmd (0x1120 + ((ch & 1) << 11) + (fg << 1) + u)
#define SPECIAL_CHAR(n)	cmd (0x1130 + ((ch & 1) << 11) + n)
#define CCODE(code, ch)	(code + ((ch & 1) << 11) + ((ch & 2) << 7))
#define RESUME_CAPTION	cmd (CCODE (0x1420, ch))
#define BACKSPACE	cmd (CCODE (0x1421, ch))
#define DELETE_EOR	cmd (CCODE (0x1424, ch))
#define ROLL_UP(rows)	cmd (CCODE (0x1425, ch) + rows - 2)
#define FLASH_ON	cmd (CCODE (0x1428, ch))
#define RESUME_DIRECT	cmd (CCODE (0x1429, ch))
#define TEXT_RESTART	cmd (CCODE (0x142A, ch))
#define RESUME_TEXT	cmd (CCODE (0x142B, ch))
#define END_OF_CAPTION	cmd (CCODE (0x142F, ch))
#define ERASE_DISPLAY	cmd (CCODE (0x142C, ch))
#define CR		cmd (CCODE (0x142D, ch))
#define ERASE_HIDDEN	cmd (CCODE (0x142E, ch))
#define TAB(t)		cmd (CCODE (0x1720, ch) + t)
#define TRANSP		(cmd (0x2000), cmd (0x172D + ((ch & 1) << 11)))
#define BLACK(u)	(cmd (0x2000), cmd (0x172E + ((ch & 1) << 11) + u))

static void
PAUSE				(unsigned int		n_frames)
{
	while (n_frames-- > 0){
		//xevent (33333);
	}
}

static void
hello_world			(void)
{
	int ch = 0;
	int i;

	pgno = -1;

	prints (" HELLO WORLD! ");
	PAUSE (30);

	ch = 4;
	TEXT_RESTART;
	prints ("Character set - Text 1");
	CR; CR;
	for (i = 32; i <= 127; i++) {
		printc (i);
		if ((i & 15) == 15)
			CR;
	}
	MIDROW (italic, 0);
	for (i = 32; i <= 127; i++) {
		printc (i);
		if ((i & 15) == 15)
			CR;
	}
	MIDROW (white, underline);
	for (i = 32; i <= 127; i++) {
		printc (i);
		if ((i & 15) == 15)
			CR;
	}
	MIDROW (white, 0);
	prints ("Special: ");
	for (i = 0; i <= 15; i++) {
		SPECIAL_CHAR (i);
	}
	CR;
	prints ("DONE - Text 1 ");
	PAUSE (50);

	ch = 5;
	TEXT_RESTART;
	prints ("Styles - Text 2");
	CR; CR;
	MIDROW (white, 0); prints ("WHITE"); CR;
	MIDROW (red, 0); prints ("RED"); CR;
	MIDROW (green, 0); prints ("GREEN"); CR;
	MIDROW (blue, 0); prints ("BLUE"); CR;
	MIDROW (yellow, 0); prints ("YELLOW"); CR;
	MIDROW (cyan, 0); prints ("CYAN"); CR;
	MIDROW (magenta, 0); prints ("MAGENTA"); BLACK (0); CR;
	BACKG (white, opaque); prints ("WHITE"); BACKG (black, opaque); CR;
	BACKG (red, opaque); prints ("RED"); BACKG (black, opaque); CR;
	BACKG (green, opaque); prints ("GREEN"); BACKG (black, opaque); CR;
	BACKG (blue, opaque); prints ("BLUE"); BACKG (black, opaque); CR;
	BACKG (yellow, opaque); prints ("YELLOW"); BACKG (black, opaque); CR;
	BACKG (cyan, opaque); prints ("CYAN"); BACKG (black, opaque); CR;
	BACKG (magenta, opaque); prints ("MAGENTA"); BACKG (black, opaque); CR;
	TRANSP;
	prints (" TRANSPARENT BACKGROUND ");
	BACKG (black, opaque); CR;
	MIDROW (white, 0); FLASH_ON;
	prints (" Flashing Text  (if implemented) "); CR;
	MIDROW (white, 0); prints ("DONE - Text 2 ");
	PAUSE (50);

	ch = 0;
	ROLL_UP (2);
	ERASE_DISPLAY;
	prints (" ROLL-UP TEST "); CR; PAUSE (20);
	prints (">> A young Jedi named Darth"); CR; PAUSE (20);
	prints ("Vader, who was a pupil of"); CR; PAUSE (20);
	prints ("mine until he turned to evil,"); CR; PAUSE (20);
	prints ("helped the Empire hunt down"); CR; PAUSE (20);
	prints ("and destroy the Jedi Knights."); CR; PAUSE (20);
	prints ("He betrayed and murdered your"); CR; PAUSE (20);
	prints ("father. Now the Jedi are all"); CR; PAUSE (20);
	prints ("but extinct. Vader was seduced"); CR; PAUSE (20);
	prints ("by the dark side of the Force."); CR; PAUSE (20);                        
	prints (">> The Force?"); CR; PAUSE (20);
	prints (">> Well, the Force is what gives"); CR; PAUSE (20);
	prints ("a Jedi his power. It's an energy"); CR; PAUSE (20);
	prints ("field created by all living"); CR; PAUSE (20);
	prints ("things."); CR; PAUSE (20);
	prints ("It surrounds us and penetrates"); CR; PAUSE (20);
	prints ("us."); CR; PAUSE (20);
	prints ("It binds the galaxy together."); CR; PAUSE (20);
	CR; PAUSE (30);
	prints (" DONE - Caption 1 ");
	PAUSE (30);

	ch = 1;
	RESUME_DIRECT;
	ERASE_DISPLAY;
	MIDROW (yellow, 0);
	INDENT (2, 10, 0); prints (" FOO "); CR;
	INDENT (3, 10, 0); prints (" MIKE WAS HERE "); CR; PAUSE (20);
	MIDROW (red, 0);
	INDENT (6, 13, 0); prints (" AND NOW... "); CR;
	INDENT (8, 13, 0); prints (" HE'S HERE "); CR; PAUSE (20);
	PREAMBLE (12, cyan, 0);
	prints ("01234567890123456789012345678901234567890123456789"); CR;
	MIDROW (white, 0);
	prints (" DONE - Caption 2 "); CR;
	PAUSE (30);
}

int
main				 (int			argc,
				 char **		argv)
{

	unsigned char  pBuffer[] ={ 
				0x00,0x01,0x02,0x00,0x15,0x00,0x00,0x00,
				0x00,0x01,0x02,0x00,0x15,0x00,0x01,0x05,
				0x00,0x01,0x02,0x00,0x15,0x00,0x41,0x40,
				0x00,0x01,0x02,0x00,0x15,0x00,0x0f,0x6a,
				0x00,0x01,0x02,0x00,0x15,0x00,0x00,0x00,
				0x00,0x01,0x02,0x00,0x15,0x00,0x00,0x00,
				0x00,0x01,0x02,0x00,0x15,0x00,0x15,0x2c, 
				0x00,0x01,0x02,0x00,0x15,0x00,0x15,0x2e, 
				0x00,0x01,0x02,0x00,0x15,0x00,0x15,0x20, 
				0x00,0x01,0x02,0x00,0x15,0x00,0x16,0x46,
				0x00,0x01,0x02,0x00,0x15,0x00,0x20,0x20,
				0x00,0x01,0x02,0x00,0x15,0x00,0x10,0x22,
				0x00,0x01,0x02,0x00,0x15,0x00,0x43,0x6c,
				0x00,0x01,0x02,0x00,0x15,0x00,0x6f,0x73,
				0x00,0x01,0x02,0x00,0x15,0x00,0x65,0x64,
				0x00,0x01,0x02,0x00,0x15,0x00,0x20,0x43,
				0x00,0x01,0x02,0x00,0x15,0x00,0x61,0x70,
				0x00,0x01,0x02,0x00,0x15,0x00,0x74,0x69,
				0x00,0x01,0x02,0x00,0x15,0x00,0x6f,0x6e ,
				
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1c,0x2f,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1c,0x29,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1c,0x29,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1d,0x6a,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1d,0x6a,

				0x00,0x00,0x02,0x00,0x1c,0x01,0x41,0x00, 

				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x20,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x20,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x45,0x00, 
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x21,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x21,

				0x00,0x00,0x02,0x00,0x1c,0x01,0x4f,0x00, 
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x22,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x22,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x55,0x00, 
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x23,

				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x23,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x55,0x00, 
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x24,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x24,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x75,0x00, 
				
				
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1c,0x2f,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1c,0x29,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1c,0x29,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1d,0x6a,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1d,0x6a,

				0x00,0x00,0x02,0x00,0x1c,0x01,0x41,0x00, 

				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x20,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x20,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x45,0x00, 
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x21,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x21,

				0x00,0x00,0x02,0x00,0x1c,0x01,0x4f,0x00, 
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x22,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x22,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x55,0x00, 
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x23,

				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x23,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x55,0x00, 
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x24,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x24,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x75,0x00, 
				
				
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1c,0x2f,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1c,0x29,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1c,0x29,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1d,0x6a,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1d,0x6a,

				0x00,0x00,0x02,0x00,0x1c,0x01,0x41,0x00, 

				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x20,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x20,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x45,0x00, 
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x21,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x21,

				0x00,0x00,0x02,0x00,0x1c,0x01,0x4f,0x00, 
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x22,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x22,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x55,0x00, 
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x23,

				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x23,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x55,0x00, 
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x24,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x24,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x75,0x00, 
				
				
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1c,0x2f,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1c,0x29,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1c,0x29,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1d,0x6a,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1d,0x6a,

				0x00,0x00,0x02,0x00,0x1c,0x01,0x41,0x00, 

				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x20,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x20,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x45,0x00, 
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x21,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x21,

				0x00,0x00,0x02,0x00,0x1c,0x01,0x4f,0x00, 
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x22,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x22,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x55,0x00, 
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x23,

				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x23,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x55,0x00, 
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x24,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x24,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x75,0x00, 
			   };

	unsigned char  pBuffer11[] ={ 1,21,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1c,0x2f,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1c,0x29,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1c,0x29,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1d,0x6a,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1d,0x6a,

				0x00,0x00,0x02,0x00,0x1c,0x01,0x41,0x00, 

				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x20,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x20,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x45,0x00, 
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x21,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x21,

				0x00,0x00,0x02,0x00,0x1c,0x01,0x4f,0x00, 
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x22,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x22,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x55,0x00, 
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x23,

				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x23,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x55,0x00, 
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x24,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x1a,0x24,
				0x00,0x00,0x02,0x00,0x1c,0x01,0x75,0x00, 

			   };

	
	int length =  N_ELEMENTS (pBuffer);
	
	
	vbi_bool success;

	//if  (!init_window (argc, argv))
	//	exit (EXIT_FAILURE);

	vbi = vbi_decoder_new ();
	
	assert (NULL != vbi);

	success = vbi_event_handler_add (vbi, VBI_EVENT_CAPTION,
					 cc_handler, /* used_data */ NULL);
	assert (success);
	
	
	success = vbi_event_handler_add (vbi, VBI_EVENT_ASPECT | VBI_EVENT_PROG_INFO,
					 xds_handler, /* used_data */ NULL);
	assert (success);
	
	
	

	//if (isatty (STDIN_FILENO)) {
	//	hello_world ();
	//} else {
		struct stream *st;

		printf("read_stream_new\n");
		st = read_stream_new (pBuffer,length,
			       FILE_FORMAT_SLICED,
			       0,
			       decode_frame,NULL);
				   
		//st = read_stream_new (/* filename: stdin */ NULL,
		//		      FILE_FORMAT_SLICED,
		//		      /* ts_pid */ 0,
		//		      decode_frame);
		stream_loop (st);
		stream_delete (st);
	//}

	printf ("Done.\n");

	//for (;;)
	//	xevent (33333);

	vbi_decoder_delete (vbi);

	exit (EXIT_SUCCESS);
}

#else /* X_DISPLAY_MISSING */

int
main				(int			argc,
				 char **		argv)
{
	printf ("Could not find X11 or has been disabled "
		"at configuration time\n");
	exit(EXIT_FAILURE);
}

#endif
