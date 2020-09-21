/*
 * AMLOGIC PTS Manager Driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the named License,
 * or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
 *
 * Author:  Tim Yao <timyao@amlogic.com>
 *
 */

#ifndef PTSSERV_H
#define PTSSERV_H

enum {
    PTS_TYPE_VIDEO = 0,
    PTS_TYPE_AUDIO = 1,
    PTS_TYPE_MAX   = 2
};

#define apts_checkin(x) pts_checkin(PTS_TYPE_AUDIO, (x))
#define vpts_checkin(x) pts_checkin(PTS_TYPE_VIDEO, (x))

extern int pts_checkin(u8 type, u32 val);

extern int pts_checkin_wrptr(u8 type, u32 ptr, u32 val);

extern int pts_checkin_offset(u8 type, u32 offset, u32 val);

extern int pts_lookup(u8 type, u32 *val, u32 pts_margin);

extern int pts_lookup_offset(u8 type, u32 offset, u32 *val, u32 pts_margin);

extern int pts_set_resolution(u8 type, u32 level);

extern int pts_set_rec_size(u8 type, u32 val);

extern int pts_start(u8 type);

extern int pts_stop(u8 type);

#endif /* PTSSERV_H */
