/******************************************************************************/
/*                                                                            */
/* Copyright (c) 2009 FUJITSU LIMITED                                         */
/*                                                                            */
/* This program is free software;  you can redistribute it and/or modify      */
/* it under the terms of the GNU General Public License as published by       */
/* the Free Software Foundation; either version 2 of the License, or          */
/* (at your option) any later version.                                        */
/*                                                                            */
/* This program is distributed in the hope that it will be useful,            */
/* but WITHOUT ANY WARRANTY;  without even the implied warranty of            */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See                  */
/* the GNU General Public License for more details.                           */
/*                                                                            */
/* You should have received a copy of the GNU General Public License          */
/* along with this program;  if not, write to the Free Software               */
/* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA    */
/*                                                                            */
/* Author: Li Zefan <lizf@cn.fujitsu.com>                                     */
/*                                                                            */
/******************************************************************************/

#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
	int i;
	pid_t pid;

	while (1) {
		for (i = 0; i < 200; i++) {
			pid = fork();
			if (pid == 0) {
				exit(0);
			} else if (pid == -1) {
				continue;
			}
		}

		for (i = 0; i < 200; i++)
			if (wait(NULL) < 0)
				break;
	}

	return 0;
}
