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
 *   WinDrv emulation functions
 *
 */
#include <ntddk.h>
#include "openpport.h"
#include "windrv.h"

static
NTSTATUS ProcessRW(
    POP_DEVICE_EXTENSION DeviceExtension,
    WINDRV_IOCTL_RW UNALIGNED *Buffer,
    ULONG  InputBufferSize);


#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, ProcessRW)
#pragma alloc_text( PAGE, WindrvDeviceControl)
#endif // ALLOC_PRAGMA


static
NTSTATUS ProcessRW(
    POP_DEVICE_EXTENSION DeviceExtension,
    WINDRV_IOCTL_RW UNALIGNED *Buffer,
    ULONG  InputBufferSize)
{
    PVOID End;
    ULONG   RwCode;
    ULONG   RwAddress;
    UCHAR   RwValue;

    if ( (0==InputBufferSize) || (0!=(InputBufferSize%sizeof(WINDRV_IOCTL_RW))) ) {
        return STATUS_INVALID_PARAMETER;
    }

    End = ((PUCHAR)Buffer) + InputBufferSize;

    while(Buffer != End)
    {

        __try {

            RwCode      = Buffer -> Code;
            RwAddress   = Buffer -> Addr;
            RwValue     = Buffer -> Value;

        } __except(EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }

        if ( (RwCode!=WINDRV_IOCTL_RW_READBYTE) && (RwCode!=WINDRV_IOCTL_RW_WRITEBYTE) ) {
            return STATUS_INVALID_PARAMETER;
        }

        //
        // Check that address belongs to allocated port range
        // and ingore the operation if it is not
        //
        if (FALSE==IsValidPortAddress(RwAddress)) {
            continue;
        }

        if (RwCode==WINDRV_IOCTL_RW_READBYTE) {

            RwValue = ReadByte(RwAddress);


            __try {

                Buffer -> Value = RwValue;

            } __except(EXCEPTION_EXECUTE_HANDLER) {
                return GetExceptionCode();
            }
        }

        if (RwCode==WINDRV_IOCTL_RW_WRITEBYTE) {

            WriteByte(RwAddress,RwValue);
        }

        Buffer ++;
    }

    return STATUS_SUCCESS;
}



NTSTATUS WindrvDeviceControl(
    POP_DEVICE_EXTENSION DeviceExtension,
    ULONG  Code,
    PVOID  InputBuffer,
    ULONG  InputBufferSize,
    PVOID  OutputBuffer,
    ULONG  OutputBufferSize,
    PULONG TransferredSize)
{
    NTSTATUS    status;

    //
    // Windrv IOCTLs use input buffer as IN/OUT buffer and 
    // return error code in output parameter. It seems that user-mode part
    // ignors the actual error code. We try to preserve this behavior
    // by setting output buffer to all zeroes in case of success,
    // and to all 0xFFs in case of error.
    //

    switch(Code)
    {

        //
        // Just do nothing on following IOCTLs
        //

    case IOCTL_WINDRV6_ENABLE:
    case IOCTL_WINDRV6_FREE_RESOURCE:

        // Ignore input parameters
        NOTHING;

        __try {
            RtlZeroMemory(OutputBuffer,OutputBufferSize);
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }

        *TransferredSize = OutputBufferSize;

        return STATUS_SUCCESS;
        break;

    case IOCTL_WINDRV6_ALLOC_RESOURCE:

        //
        // We blindly ignore this IOCTL. We need to set a 
        // "resource handle" to non-zero though
        //
        __try {

            ((WINDRV_IOCTL_RSRC   UNALIGNED *)InputBuffer)->Handle = 0x1234;

            RtlZeroMemory(OutputBuffer,OutputBufferSize);
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }

        *TransferredSize = OutputBufferSize;

        return STATUS_SUCCESS;
        break;

    case IOCTL_WINDRV6_READWRITE:

        if (InputBufferSize < sizeof(WINDRV_IOCTL_RW)) {
            return STATUS_INVALID_PARAMETER;
        }

        status = ProcessRW(DeviceExtension,InputBuffer,sizeof(WINDRV_IOCTL_RW));
        if (!NT_SUCCESS(status)) {
            return status;
        }

        //
        // Indicate success
        // 
        __try {
            RtlZeroMemory(OutputBuffer,OutputBufferSize);
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }

        *TransferredSize = OutputBufferSize;
        return STATUS_SUCCESS;
        break;

    case IOCTL_WINDRV6_READWRITE_ARRAY:

        status = ProcessRW(DeviceExtension,InputBuffer,InputBufferSize);

        if (!NT_SUCCESS(status)) {
            return status;
        }

        //
        // Indicate success
        // 
        __try {
            RtlZeroMemory(OutputBuffer,OutputBufferSize);
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }

        *TransferredSize = OutputBufferSize;
        return STATUS_SUCCESS;
        break;
    }

    KdPrint(( "OpenParport: Unknown IOCTL=%08x received by windrv emulator\n",Code));

    return STATUS_NOT_IMPLEMENTED;
}
