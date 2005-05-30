/*
 * Copyright (C) 2005 Andrey V Lelikov
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * Contents:
 *   
 *   /dev/ppdev Win32-specific user-mode emulation code
 *
 */
#include <windows.h>

#ifndef _IOW
#define _IOW(a,b,c) (b)
#endif

#ifndef _IOR
#define _IOR(a,b,c) (b)
#endif

#ifndef _IO
#define _IO(a,b) (b)
#endif


#define OBSOLETE__IOW(a,b,c)    _IOW(a,b,c)
#define OBSOLETE__IOR(a,b,c)    _IOR(a,b,c)

int PP_open(const char *filename,int oflag,...);
int PP_close(int handle);
int PP_ioctl(int handle,int code,...);

