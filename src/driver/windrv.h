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
 *   WinDrv emulation header file
 *
 */


#define WINDRV_DEVICE_NAME  L"\\Device\\OpenParportWindrv6Emulator"
#define WINDRV_SYMLINK_NAME L"\\DosDevices\\WINDRVR6"

//
// Relevant CTL codes for WINDRV6 driver
//

#define FILE_DEVICE_WINDRV6 0x9538


#define IOCTL_WINDRV6_ENABLE \
    CTL_CODE(FILE_DEVICE_WINDRV6,0x952,METHOD_NEITHER,FILE_ANY_ACCESS)

C_ASSERT( ((ULONG)(IOCTL_WINDRV6_ENABLE)) == 0x9538254B );

#define IOCTL_WINDRV6_ALLOC_RESOURCE \
    CTL_CODE(FILE_DEVICE_WINDRV6,0x97D,METHOD_NEITHER,FILE_ANY_ACCESS)
C_ASSERT( ((ULONG)(IOCTL_WINDRV6_ALLOC_RESOURCE)) == 0x953825F7 );

#define IOCTL_WINDRV6_FREE_RESOURCE \
    CTL_CODE(FILE_DEVICE_WINDRV6,0x92B,METHOD_NEITHER,FILE_ANY_ACCESS)
C_ASSERT( ((ULONG)(IOCTL_WINDRV6_FREE_RESOURCE)) == 0x953824AF );

#define IOCTL_WINDRV6_READWRITE \
    CTL_CODE(FILE_DEVICE_WINDRV6,0x903,METHOD_NEITHER,FILE_ANY_ACCESS)
C_ASSERT( ((ULONG)(IOCTL_WINDRV6_READWRITE)) == 0x9538240F );

#define IOCTL_WINDRV6_READWRITE_ARRAY \
    CTL_CODE(FILE_DEVICE_WINDRV6,0x904,METHOD_NEITHER,FILE_ANY_ACCESS)
C_ASSERT( ((ULONG)(IOCTL_WINDRV6_READWRITE_ARRAY)) == 0x95382413 );



//
// Structures related to CTL codes
//

#include <pshpack1.h>

typedef struct _WINDRV_IOCTL_RSRC
{
    UCHAR               _unknown0[0x328];
    ULONG               Handle;
    UCHAR               _unknown1[0x3b4 - (sizeof(ULONG)+0x328) ];
} WINDRV_IOCTL_RSRC,*PWINDRV_IOCTL_RSRC;

C_ASSERT(FIELD_OFFSET(WINDRV_IOCTL_RSRC,Handle)==0x328);
C_ASSERT(sizeof(WINDRV_IOCTL_RSRC) == 0x3b4);

typedef struct _WINDRV_IOCTL_RW
{
    ULONG   Addr;
    ULONG   Code;
    ULONG   _unknown0[3];
    UCHAR   Value;
    UCHAR   _valuepad[3];
    ULONG   _unknown1;
} WINDRV_IOCTL_RW,*PWINDRV_IOCTL_RW;

#define WINDRV_IOCTL_RW_READBYTE    0x0A
#define WINDRV_IOCTL_RW_WRITEBYTE   0x0D

#include <poppack.h>

