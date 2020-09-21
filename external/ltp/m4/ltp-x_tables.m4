dnl Copyright (c) 2017 Richard Palethorpe <rpalethorpe@suse.com>
dnl
dnl This program is free software;  you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 2 of the License, or
dnl (at your option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY;  without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
dnl the GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program. If not, see <http://www.gnu.org/licenses/>.

AC_DEFUN([LTP_CHECK_X_TABLES],[
AC_CHECK_TYPES([struct xt_entry_match, struct xt_entry_target],,,[
#include <sys/types.h>
#include <netinet/in.h>
#include <net/if.h>
#include <limits.h>
#include <linux/netfilter_ipv4/ip_tables.h>
])
])
