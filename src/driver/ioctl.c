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
 *   IOCTL processing module
 *
 */
#include <ntddk.h>
#include "openpport.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, MainDeviceControl)
#endif // ALLOC_PRAGMA


#define VER_STRING "OpenParport"

NTSTATUS MainDeviceControl(
    POP_DEVICE_EXTENSION DeviceExtension,
    ULONG  Code,
    PVOID  InputBuffer,
    ULONG  InputBufferSize,
    PVOID  OutputBuffer,
    ULONG  OutputBufferSize,
    PULONG TransferredSize)
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG    RequiredOutSize;

    OP_IOCTL_VERSION_INFO UNALIGNED *pVersionInfo;
    OP_IOCTL_READ_BYTE      rb;
    OP_IOCTL_WRITE_BYTE     wb;
    UCHAR                   Byte;

    //
    // First of all, check if this is an emulated IOCTL.
    // If it is, call corresponding
    // dispatch routine and exit
    //

    if (DeviceExtension->Type==gpdWindrv) {
        return WindrvDeviceControl(
            DeviceExtension,
            Code,
            InputBuffer,
            InputBufferSize,
            OutputBuffer,
            OutputBufferSize,
            TransferredSize);
    }

    //
    // Process "native" IOCTLs
    //

    switch(Code)
    {
    case IOCTL_OP_IOCTL_GET_VERSION:

        RequiredOutSize=sizeof(OP_IOCTL_VERSION_INFO) + sizeof(VER_STRING);

        if (OutputBufferSize < RequiredOutSize) {
            return STATUS_BUFFER_TOO_SMALL;
        }

        pVersionInfo = (POP_IOCTL_VERSION_INFO) OutputBuffer;

        __try {
            pVersionInfo->VersionMajor = VER_MAJOR;
            pVersionInfo->VersionMinor = VER_MINOR;

            RtlCopyMemory(
                pVersionInfo->VersionString,
                VER_STRING,
                sizeof(VER_STRING));

        } __except(EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }

        *TransferredSize = RequiredOutSize;

        return STATUS_SUCCESS;

    case IOCTL_OP_IOCTL_READ_BYTE:

        if (InputBufferSize < sizeof(OP_IOCTL_READ_BYTE)) {
            return STATUS_INVALID_PARAMETER;
        }

        if (OutputBufferSize == 0) {
            return STATUS_INVALID_PARAMETER;
        }

        __try {
            RtlCopyMemory(
                &rb,
                InputBuffer,
                sizeof(rb));
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }

        if (FALSE==IsValidPortAddress(rb.Address)) {
            return STATUS_NO_SUCH_DEVICE;
        }

        Byte = ReadByte(rb.Address);

        __try {
            RtlCopyMemory(
                OutputBuffer,
                &Byte,
                sizeof(Byte));
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }

        *TransferredSize = sizeof(Byte);

        return STATUS_SUCCESS;

    case IOCTL_OP_IOCTL_WRITE_BYTE:

        if (InputBufferSize < sizeof(OP_IOCTL_WRITE_BYTE)) {
            return STATUS_INVALID_PARAMETER;
        }

        __try {
            RtlCopyMemory(
                &wb,
                InputBuffer,
                sizeof(wb));
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }

        if (FALSE==IsValidPortAddress(wb.Address)) {
            return STATUS_NO_SUCH_DEVICE;
        }

        WriteByte(wb.Address,wb.Value);

        *TransferredSize = 0;

        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_DEVICE_REQUEST;
}
