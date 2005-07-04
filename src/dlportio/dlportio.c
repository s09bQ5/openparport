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
#include <ppapi.h>

static ULONG  DeviceBase=0;
static HANDLE Device=NULL;
static UCHAR  CtrlShadow=0;
static UCHAR  DataShadow=0;

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
                     )
{
    if (DLL_PROCESS_ATTACH==ul_reason_for_call) {
        DisableThreadLibraryCalls(hModule);
    }
    return TRUE;
}

BOOL OpenDevice(IN ULONG Port)
{
    LONG PortId=-1;

    if (NULL!=Device)
        return TRUE;

#if DBG
    __debugbreak();
#endif

    //
    // Guess which device to open. Currnt map:
    // 0x378 - 0
    // 0x278 - 1
    //

    if ( (Port>=0x378) && (Port<(0x378+4)) ) {
        PortId = 0;
        DeviceBase = 0x378;
    }
    if ( (Port>=0x278) && (Port<(0x278+4)) ) {
        PortId = 1;
        DeviceBase = 0x278;
    }

    Device=PPOpen(PortId);

    return (NULL!=Device);
}


UCHAR WINAPI DlPortReadPortUchar(IN ULONG Port)
{
    UCHAR Value;
    ULONG Offset;

    if (FALSE==OpenDevice(Port)) {
        return 0;
    }

    Offset = Port - DeviceBase;

    switch(Offset)
    {
    case PP_OFFSET_STATUS:
        PPRead(Device,&Value);
        break;
    case PP_OFFSET_CONTROL:
        Value = CtrlShadow;
        break;
    case PP_OFFSET_DATA:
        Value = DataShadow;
        break;
    default:
        Value = 0;
        break;
    }

    return Value;
}

VOID WINAPI DlPortWritePortUchar(IN ULONG Port,IN UCHAR Value)
{
    ULONG Offset;

    if (FALSE==OpenDevice(Port)) {
        return;
    }

    Offset = Port - DeviceBase;

    switch(Offset)
    {
    case PP_OFFSET_DATA:
        PPWrite(Device, PP_OFFSET_DATA ,Value);
        DataShadow = Value;
        break;
    case PP_OFFSET_CONTROL:
        PPWrite(Device, PP_OFFSET_CONTROL ,Value);
        CtrlShadow = Value;
        break;
    default:
        break;
    }
}

