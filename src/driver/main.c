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
 *   Main driver module
 *
 */
#include <ntddk.h>
#include "openpport.h"
#include "windrv.h"

NTSTATUS DriverEntry(IN PDRIVER_OBJECT   DriverObject,IN PUNICODE_STRING  RegPath);
NTSTATUS DispatchCreateClose(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp);
NTSTATUS DispatchDeviceControl(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp);
VOID     DriverUnload(IN PDRIVER_OBJECT DriverObject);

NTSTATUS CreateDevice(
    POP_DEVICE_EXTENSION *pDeviceExtension,
    PDRIVER_OBJECT DriverObject,
    LPCWSTR DeviceName,
    LPCWSTR SymlinkName,
    OP_DEVICE_TYPE Type,
    DEVICE_TYPE DeviceType,
    ULONG ExtensionSize
    );

BOOLEAN DispatchFastIoDeviceControl(
    IN struct _FILE_OBJECT *FileObject,
    IN BOOLEAN Wait,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, DriverEntry )
#pragma alloc_text( PAGE, DriverUnload)
#pragma alloc_text( PAGE, DispatchCreateClose)
#pragma alloc_text( PAGE, DispatchDeviceControl)
#pragma alloc_text( PAGE, DispatchFastIoDeviceControl)
#pragma alloc_text( PAGE, CreateDevice)
#endif // ALLOC_PRAGMA

static FAST_IO_DISPATCH FastIoDispatch;


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  RegPath
    )
{
    NTSTATUS status;

    //
    // Set up dispatch routines
    //

    DriverObject->MajorFunction[ IRP_MJ_CREATE                  ] = DispatchCreateClose;
    DriverObject->MajorFunction[ IRP_MJ_CLOSE                   ] = DispatchCreateClose;
    DriverObject->MajorFunction[ IRP_MJ_DEVICE_CONTROL          ] = DispatchDeviceControl;
    DriverObject->DriverUnload                                    = DriverUnload;

    //
    // Set up FastIo IOCTL dispatch
    //
    RtlZeroMemory(&FastIoDispatch,sizeof(FastIoDispatch));
    FastIoDispatch.SizeOfFastIoDispatch = sizeof(FAST_IO_DISPATCH);
    FastIoDispatch.FastIoDeviceControl = DispatchFastIoDeviceControl;

    DriverObject->FastIoDispatch = &FastIoDispatch;

    // create main device
    status = CreateDevice(
        NULL,
        DriverObject,
        L"\\Device\\OpenParportMain",
        L"\\DosDevices\\OpenParport",
        gpdMain,
        FILE_DEVICE_OPENPPORT,
        sizeof(OP_DEVICE_EXTENSION)
        );

    if ( !NT_SUCCESS( status ) )
    {
        DriverUnload(DriverObject);
        return status;
    }

    // create windrv device
    status = CreateDevice(
        NULL,
        DriverObject,
        WINDRV_DEVICE_NAME,
        WINDRV_SYMLINK_NAME,
        gpdWindrv,
        FILE_DEVICE_WINDRV6,
        sizeof(OP_DEVICE_EXTENSION)
        );

    if ( !NT_SUCCESS( status ) )
    {
        DriverUnload(DriverObject);
        return status;
    }

    return STATUS_SUCCESS;
}

/*
    Create/Close - Do nothing
*/
NTSTATUS
DispatchCreateClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    
    return STATUS_SUCCESS;
}

VOID
DriverUnload(
    IN PDRIVER_OBJECT       DriverObject
    )
{
    PDEVICE_OBJECT          DeviceObject;
    POP_DEVICE_EXTENSION    DeviceExtension;
    UNICODE_STRING          String;

    while(NULL!=(DeviceObject=DriverObject->DeviceObject)) {

        DeviceExtension = (POP_DEVICE_EXTENSION) DeviceObject->DeviceExtension;

        //
        // Delete symbolic link
        //
        if (FALSE!=DeviceExtension->SymlinkCreated) {
            DeviceExtension->SymlinkCreated = FALSE;

            RtlInitUnicodeString(&String,DeviceExtension->SymlinkName);
            IoDeleteSymbolicLink(&String);
        }

        //
        // Free symlink name
        //
        if (NULL!=DeviceExtension->SymlinkName) {
            ExFreePool(DeviceExtension->SymlinkName);
        }

        //
        // Release port resources
        //
//        if (0!=DeviceExtension->PortAddress) {
//            ReleasePort(DeviceExtension);
//        }

        // 
        // Delete Device
        //
        IoDeleteDevice(DeviceObject);
    }

}

NTSTATUS CreateDevice(
    POP_DEVICE_EXTENSION *pDeviceExtension,
    PDRIVER_OBJECT DriverObject,
    LPCWSTR DeviceName,
    LPCWSTR SymlinkName,
    OP_DEVICE_TYPE Type,
    DEVICE_TYPE DeviceType,
    ULONG ExtensionSize
    )
{
    NTSTATUS status;
    UNICODE_STRING  DeviceNameStr;
    UNICODE_STRING  SymlinkNameStr;
    PDEVICE_OBJECT  DeviceObject;
    POP_DEVICE_EXTENSION    Extension;
    ULONG           SymlinkNameLen;

    RtlInitUnicodeString( &DeviceNameStr, DeviceName );
    
    status = IoCreateDevice(
        DriverObject,
        ExtensionSize,
        &DeviceNameStr,
        DeviceType,  
        FILE_DEVICE_SECURE_OPEN,
        FALSE,                  
        &DeviceObject );    

    if ( !NT_SUCCESS( status ) )
    {
        return status;
    }

    Extension=DeviceObject->DeviceExtension;

    RtlZeroMemory(Extension,ExtensionSize);

    Extension->Type = Type;
    Extension->DeviceObject = DeviceObject;

    // copy symlink name
    SymlinkNameLen = (ULONG)((wcslen(SymlinkName)+1)*sizeof(WCHAR));

    Extension->SymlinkName = ExAllocatePoolWithTag(PagedPool,SymlinkNameLen,OP_MEM_TAG);
    if (NULL==Extension->SymlinkName) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(Extension->SymlinkName,SymlinkName,SymlinkNameLen);

    RtlInitUnicodeString(&SymlinkNameStr,Extension->SymlinkName);

    // create symlink
    status = IoCreateSymbolicLink(
        &SymlinkNameStr, 
        &DeviceNameStr);

    if ( !NT_SUCCESS( status ) )
    {
        return status;
    }

    Extension->SymlinkCreated = TRUE;

    if (NULL!=pDeviceExtension) 
    {
        *pDeviceExtension = Extension;
    }

    return STATUS_SUCCESS;
}

BOOLEAN DispatchFastIoDeviceControl(
    IN struct _FILE_OBJECT *FileObject,
    IN BOOLEAN Wait,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    )
{
    NTSTATUS    status;
    ULONG       TransferredBytes=0;

    //
    // All of our CTL codes are compatible with FastIo
    //

    //
    // Do buffers verification
    //
    __try {
        if (0!=InputBufferLength) {
            ProbeForRead(InputBuffer,InputBufferLength,1);
        }
        if (0!=OutputBufferLength) {
            ProbeForRead(OutputBuffer,OutputBufferLength,1);
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
        goto exit;
    }

    status = MainDeviceControl(
        (POP_DEVICE_EXTENSION)DeviceObject->DeviceExtension,
        IoControlCode,
        InputBuffer,
        InputBufferLength,
        OutputBuffer,
        OutputBufferLength,
        &TransferredBytes);

    if (NT_SUCCESS(status)) {
        IoStatus->Information = TransferredBytes;
    }

exit:
    IoStatus->Status = status;

    return TRUE;
}



NTSTATUS DispatchDeviceControl(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp)
{
    PIO_STACK_LOCATION  Stack;
    NTSTATUS            status;
    ULONG               Method;
    ULONG               TransferredBytes=0;

    Stack = IoGetCurrentIrpStackLocation( Irp );

    Method = (Stack->Parameters.DeviceIoControl.IoControlCode) & 3;

    if ( (Method!=METHOD_BUFFERED) && (Method!=METHOD_NEITHER) ) {
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto exit;
    }

    if (Method==METHOD_BUFFERED) {

        // METHOD_BUFFERED
        status = MainDeviceControl(
            (POP_DEVICE_EXTENSION)DeviceObject->DeviceExtension,
            Stack->Parameters.DeviceIoControl.IoControlCode,
            Irp->AssociatedIrp.SystemBuffer,
            Stack->Parameters.DeviceIoControl.InputBufferLength,
            Irp->AssociatedIrp.SystemBuffer,
            Stack->Parameters.DeviceIoControl.OutputBufferLength,
            &TransferredBytes);

        if (NT_SUCCESS(status)) {
            Irp->IoStatus.Information = TransferredBytes;
        }

    } else {

        // METHOD_NEITHER

        //
        // Do buffers verification
        //
        __try {
        if (0!=Stack->Parameters.DeviceIoControl.InputBufferLength) {
            ProbeForRead(Stack->Parameters.DeviceIoControl.Type3InputBuffer,Stack->Parameters.DeviceIoControl.InputBufferLength,1);
        }
        if (0!=Stack->Parameters.DeviceIoControl.OutputBufferLength) {
            ProbeForRead(Irp->UserBuffer,Stack->Parameters.DeviceIoControl.OutputBufferLength,1);
        }
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            status = GetExceptionCode();
            goto exit;
        }

        status = MainDeviceControl(
            (POP_DEVICE_EXTENSION)DeviceObject->DeviceExtension,
            Stack->Parameters.DeviceIoControl.IoControlCode,
            Stack->Parameters.DeviceIoControl.Type3InputBuffer,
            Stack->Parameters.DeviceIoControl.InputBufferLength,
            Irp->UserBuffer,
            Stack->Parameters.DeviceIoControl.OutputBufferLength,
            &TransferredBytes);

        if (NT_SUCCESS(status)) {
            Irp->IoStatus.Information = TransferredBytes;
        }

    }


exit:
    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return status;
}

