/*
 * Copyright (C) 2005 Andrey V Lelikov
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with the GNU C Library; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307 USA.  
 *
 * Contents:
 *   
 *   DriverLINX emulation
 *
 */
#include <windows.h>
#include <winioctl.h>
#include <opioctl.h>

static HANDLE Device=NULL;

static
HANDLE WINAPI OPOpen()
{
    HANDLE h = CreateFileW(
        OP_UM_DEVICE_NAME,
        GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);


    return h;
}

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
                     )
{
    if (DLL_PROCESS_ATTACH==ul_reason_for_call) {

        Device = OPOpen();

        if (INVALID_HANDLE_VALUE==Device) {
            return FALSE;
        }

    }
    return TRUE;
}


UCHAR WINAPI DlPortReadPortUchar(IN ULONG Port)
{
    OP_IOCTL_READ_BYTE  rb;
    DWORD   dwTransferred;
    UCHAR   Value=0;

    rb.Address = (USHORT) Port;

    DeviceIoControl(Device,IOCTL_OP_IOCTL_READ_BYTE,&rb,sizeof(rb),&Value,sizeof(Value),&dwTransferred,NULL);

    return Value;
}

VOID WINAPI DlPortWritePortUchar(IN ULONG Port,IN UCHAR Value)
{
    OP_IOCTL_WRITE_BYTE  wb;
    DWORD   dwTransferred;

    wb.Address = (USHORT) Port;
    wb.Value = Value;

    DeviceIoControl(Device,IOCTL_OP_IOCTL_WRITE_BYTE,&wb,sizeof(wb),NULL,0,&dwTransferred,NULL);
}

