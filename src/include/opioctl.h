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
 *   IOCTL interface to OpenParport driver
 *
 *
 */
#ifndef OPIOCTL_H_INCLUDED
#define OPIOCTL_H_INCLUDED


#define FILE_DEVICE_OPENPPORT 0x83EB

#define OP_UM_DEVICE_NAME   L"\\\\.\\OpenParport"

typedef struct _OP_IOCTL_VERSION_INFO
{
    UCHAR   VersionMajor;
    UCHAR   VersionMinor;
    UCHAR   VersionString[1];
} OP_IOCTL_VERSION_INFO,*POP_IOCTL_VERSION_INFO;

#define IOCTL_OP_IOCTL_GET_VERSION \
        CTL_CODE(FILE_DEVICE_OPENPPORT,0x900,METHOD_NEITHER,FILE_ANY_ACCESS)


typedef struct _OP_IOCTL_READ_BYTE
{
    USHORT  Address;
} OP_IOCTL_READ_BYTE,*POP_IOCTL_READ_BYTE;

#define IOCTL_OP_IOCTL_READ_BYTE \
        CTL_CODE(FILE_DEVICE_OPENPPORT,0x901,METHOD_NEITHER,FILE_ANY_ACCESS)


typedef struct _OP_IOCTL_WRITE_BYTE
{
    USHORT  Address;
    UCHAR   Value;
} OP_IOCTL_WRITE_BYTE,*POP_IOCTL_WRITE_BYTE;

#define IOCTL_OP_IOCTL_WRITE_BYTE \
        CTL_CODE(FILE_DEVICE_OPENPPORT,0x902,METHOD_NEITHER,FILE_ANY_ACCESS)

#endif // OPIOCTL_H_INCLUDED
