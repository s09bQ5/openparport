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
 *   User-mode API for direct parallel port access
 *
 */
#include <windows.h>
#include <winioctl.h>
#include <winerror.h>

#define IOCTL_PP_WRITE_DATA         0x002C0004
#define IOCTL_PP_WRITE_CONTROL      0x002C0008
#define IOCTL_PP_READ_STATUS        0x002C000C

#define PP_DEVICE_NAME              L"\\\\.\\$VDMLPT%d"
#define PP_DEVICE_NAME_SIZE         (sizeof(PP_DEVICE_NAME)+20*sizeof(WCHAR))

#define PP_OFFSET_DATA      0
#define PP_OFFSET_STATUS    1
#define PP_OFFSET_CONTROL   2

HANDLE __inline WINAPI PPOpen(ULONG DeviceId)
{
    WCHAR DeviceName[PP_DEVICE_NAME_SIZE];
    HANDLE h;

    wsprintfW(DeviceName,PP_DEVICE_NAME,DeviceId+1);

    h = CreateFileW(
        DeviceName,
        GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    if (INVALID_HANDLE_VALUE==h) {
        h = NULL;
    }

    return h;
}

BOOL __inline WINAPI PPWrite(HANDLE hDevice,UCHAR Offset,UCHAR Value)
{
    ULONG CtlCode;
    ULONG Transferred;
    ULONG FakeByteForDriverBug;

    if ( (Offset!=PP_OFFSET_DATA) && (Offset!=PP_OFFSET_CONTROL) ) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    CtlCode = (Offset==PP_OFFSET_DATA) ? IOCTL_PP_WRITE_DATA : IOCTL_PP_WRITE_CONTROL;

    return DeviceIoControl(
        hDevice,
        CtlCode,
        &Value,
        sizeof(Value),
        &FakeByteForDriverBug,
        sizeof(FakeByteForDriverBug),
        &Transferred,
        NULL);
}

BOOL __inline WINAPI PPRead(HANDLE hDevice,PUCHAR pValue)
{
    ULONG Transferred;

    return DeviceIoControl(
        hDevice,
        IOCTL_PP_READ_STATUS,
        NULL,
        0,
        pValue,
        sizeof(UCHAR),
        &Transferred,
        NULL);
}
