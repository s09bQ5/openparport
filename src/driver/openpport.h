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
 *   Internal driver functions / structures
 *
 */
#include "opioctl.h"

#define OP_MEM_TAG 'rapO'

//
// Type of device object
//
typedef enum _OP_DEVICE_TYPE
{
    gpdMain,
    gpdWindrv,
    gpdMaxValue
} OP_DEVICE_TYPE;

typedef struct _OP_DEVICE_EXTENSION
{
    PDEVICE_OBJECT  DeviceObject;
    OP_DEVICE_TYPE  Type;

    PWSTR           SymlinkName;
    BOOLEAN         SymlinkCreated;
} OP_DEVICE_EXTENSION,*POP_DEVICE_EXTENSION;

//
// Valid port ranges are hardcoded here
//
BOOLEAN __inline IsValidPortAddress(ULONG Port)
{
    if ( (Port>=0x378) && (Port<0x37c) ) return TRUE;
    if ( (Port>=0x278) && (Port<0x27c) ) return TRUE;

    return FALSE;
}


//
// Internal functions
//

NTSTATUS MainDeviceControl(
    POP_DEVICE_EXTENSION DeviceExtension,
    ULONG  Code,
    PVOID  InputBuffer,
    ULONG  InputBufferSize,
    PVOID  OutputBuffer,
    ULONG  OutputBufferSize,
    PULONG TransferredSize);


NTSTATUS WindrvDeviceControl(
    POP_DEVICE_EXTENSION DeviceExtension,
    ULONG  Code,
    PVOID  InputBuffer,
    ULONG  InputBufferSize,
    PVOID  OutputBuffer,
    ULONG  OutputBufferSize,
    PULONG TransferredSize);

VOID __inline WriteByte(
    ULONG Address,
    UCHAR Value
    )
{
    WRITE_PORT_UCHAR( (PUCHAR) (ULONG_PTR) (Address) , Value );
#ifdef TRACEIO
    KdPrint(( "GP write %04X %2X\n",Address,Value));
#endif
}

UCHAR __inline ReadByte(
    ULONG Address
    )
{
    UCHAR Value;
    Value = READ_PORT_UCHAR( (PUCHAR) (ULONG_PTR) (Address) );
#ifdef TRACEIO
    KdPrint(( "GP read  %04X %2X\n",Address,Value));
#endif
    return Value;
}

