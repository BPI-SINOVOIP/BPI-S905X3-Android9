/*
 *  libzvbi -- Teletext decoder internals
 *
 *  Copyright (C) 2000, 2001, 2008 Michael H. Schimek
 *
 *  Originally based on AleVT 1.5.1 by Edgar Toernig
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301  USA.
 */

/* $Id: teletext_decoder.h,v 1.2 2008/02/19 00:35:22 mschimek Exp $ */

#ifndef TELETEXT_H
#define TELETEXT_H

#include "cache-priv.h"

struct raw_page {
	cache_page		page[1];
	uint8_t			drcs_mode[48];
	int			num_triplets;
	int			ait_page;
};

/* Public */

/**
 * @ingroup HiDec
 * @brief Teletext implementation level.
 */
typedef enum {
	VBI_WST_LEVEL_1,   /**< 1 - Basic Teletext pages */
	VBI_WST_LEVEL_1p5, /**< 1.5 - Additional national and graphics characters */
	/**
	 * 2.5 - Additional text styles, more colors and DRCS. You should
	 * enable Level 2.5 only if you can render and/or export such pages.
	 */
	VBI_WST_LEVEL_2p5,
	VBI_WST_LEVEL_3p5  /**< 3.5 - Multicolor DRCS, proportional script */
} vbi_wst_level;

/**\Teletext subtitle mode*/
typedef enum {
    VBI_TELETEXT_NON_BITMAP_SUB = -1,     /** NON bitmap subtitle*/
    VBI_TELETEXT_BITMAP_SUB     = 1,      /**< ttx bitmap subtitle*/
}vbi_submode;
/* Private */

struct teletext {
	vbi_wst_level			max_level;

	struct ttx_page_link		header_page;
	uint8_t		        	header[40];

	struct ttx_magazine		default_magazine;

	int                     	region;

	struct raw_page			raw_page[8];
	struct raw_page			*current;
	vbi_pgno current_pgno;
	vbi_subno current_subno;
	vbi_bool subtitle;
	vbi_bool use_subtitleserver;
	vbi_submode subtitleMode;
	int goto_page;
};

/* Public */

/**
 * @addtogroup Service
 * @{
 */
extern void		vbi_teletext_set_default_region(vbi_decoder *vbi, int default_region);
extern void		vbi_teletext_set_level(vbi_decoder *vbi, int level);
/** @} */
/**
 * @addtogroup Cache
 * @{
 */
extern void vbi_set_subtitle_flag(vbi_decoder *vbi, int flag, int subtitleMode, vbi_bool useSubtitleserver);
extern void vbi_set_subtitle_page(vbi_decoder *vbi, int index);
extern void vbi_teletext_set_current_page(vbi_decoder *vbi, vbi_pgno pgno, vbi_subno subno);
extern vbi_bool		vbi_fetch_vt_page(vbi_decoder *vbi, vbi_page *pg,
					  vbi_pgno pgno, vbi_subno subno,
					  vbi_wst_level max_level, int display_rows,
					  vbi_bool navigation, int* page_type);
extern int		vbi_page_title(vbi_decoder *vbi, int pgno, int subno, char *buf);
/** @} */
/**
 * @addtogroup Event
 * @{
 */
extern void		vbi_resolve_link(vbi_page *pg, int column, int row,
					 vbi_link *ld);
extern void		vbi_resolve_home(vbi_page *pg, vbi_link *ld);
/** @} */

/* Private */

/* packet.c */

extern void		vbi_teletext_init(vbi_decoder *vbi);
extern void		vbi_teletext_destroy(vbi_decoder *vbi);
extern vbi_bool		vbi_decode_teletext(vbi_decoder *vbi, uint8_t *p);
extern void		vbi_teletext_desync(vbi_decoder *vbi);
extern void             vbi_teletext_channel_switched(vbi_decoder *vbi);
extern cache_page *	vbi_convert_page(vbi_decoder *vbi, cache_page *vtp,
					 vbi_bool cached,
					 enum ttx_page_function new_function);

extern void		vbi_decode_vps(vbi_decoder *vbi, uint8_t *p);

/* teletext.c */

extern vbi_bool		vbi_format_vt_page(vbi_decoder *, vbi_page *,
					   cache_page *,
					   vbi_wst_level max_level,
					   int display_rows,
					   vbi_bool navigation);

extern vbi_bool         vbi_get_next_pgno(vbi_decoder *vbi, int dir, vbi_pgno *pgno, vbi_pgno *subno);

#endif

/*
Local variables:
c-set-style: K&R
c-basic-offset: 8
End:
*/
