#! /bin/sh
# Copyright (C) 2015 Red Hat, Inc.
# This file is part of elfutils.
#
# This file is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# elfutils is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

. $srcdir/test-subr.sh

testfiles debug-ranges-no-lowpc.o

testrun_compare ${abs_builddir}/dwarf-ranges debug-ranges-no-lowpc.o 0xb <<\EOF
1..2 (base 0)
3..4 (base 0)
EOF

exit 0
