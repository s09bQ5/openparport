/*
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
 * Copyright (C) 2005 Andrey V Lelikov
 *
 * Contents:
 *   
 *   Main driver module
 *
 */
#include <ntddk.h>
#include <wxp\ntimage.h>

NTSTATUS DriverEntry(IN PDRIVER_OBJECT   DriverObject,IN PUNICODE_STRING  RegPath);
NTSTATUS DispatchCreateClose(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp);
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

extern POBJECT_TYPE *IoDriverObjectType;

// http://www.google.com/search?hl=en&q=ObReferenceObjectByName
NTSYSAPI
NTSTATUS NTAPI ObReferenceObjectByName(
    IN PUNICODE_STRING ObjectPath,
    IN ULONG Attributes,
    IN PACCESS_STATE PassedAccessState OPTIONAL,
    IN ACCESS_MASK DesiredAccess OPTIONAL,
    IN POBJECT_TYPE ObjectType,
    IN KPROCESSOR_MODE AccessMode,
    IN OUT PVOID ParseContext OPTIONAL,
    OUT PVOID *ObjectPtr);

static 
VOID
FASTCALL
MyIofCompleteRequest(
    IN PIRP Irp,
    IN CCHAR PriorityBoost
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, DriverEntry )
#pragma alloc_text( PAGE, DispatchCreateClose)
#pragma alloc_text( PAGE, DispatchFastIoDeviceControl)
#endif // ALLOC_PRAGMA

static FAST_IO_DISPATCH FastIoDispatch;

#define SPECIAL_CANCEL_MARK ( (PDRIVER_CANCEL) (ULONG_PTR) 0x12345 )

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  RegPath
    )
{
    NTSTATUS status=STATUS_SUCCESS;
    PDRIVER_OBJECT  ParvdmDriver=NULL;
    PIMAGE_DOS_HEADER   DosHeader;
    PIMAGE_NT_HEADERS   NtHeader;
    PIMAGE_SECTION_HEADER   IdataSection;
    ULONG   i,found,flags;
    PVOID   IofCompleteRequestAdr,*ImportStart,*ImportEnd,*IofCompleteRequestImportPtr,*IofCompleteRequestImportPtrRw;
    UNICODE_STRING  us;

    struct _ImportMdl
    {
        MDL     Mdl;
        ULONG   Pages[2];
    } ImportMdl;

#if DBG
    __debugbreak();
#endif

    //
    // We have to _call_ this function. Its a compiler bug.
    //
    MyIofCompleteRequest(NULL,0);

    //
    // Set up dispatch routines
    //

    DriverObject->MajorFunction[ IRP_MJ_CREATE                  ] = DispatchCreateClose;
    DriverObject->MajorFunction[ IRP_MJ_CLOSE                   ] = DispatchCreateClose;

    //
    // Locate parvdm driver object
    //
    RtlInitUnicodeString(&us,L"\\Driver\\parvdm");

    status = ObReferenceObjectByName(
        &us,
        OBJ_CASE_INSENSITIVE,
        NULL,
        0,
        *IoDriverObjectType,
        KernelMode,
        NULL,
        &ParvdmDriver);

    if (!NT_SUCCESS(status)) {
        ParvdmDriver = NULL;
        status = STATUS_NO_SUCH_DEVICE;
        goto exit;
    }

    //
    // Locate import section
    //
    DosHeader = ParvdmDriver->DriverStart;
    NtHeader = (PIMAGE_NT_HEADERS) ( ((PUCHAR)DosHeader) + DosHeader->e_lfanew );

    found = 0;
    flags = IMAGE_SCN_MEM_NOT_PAGED | IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_INITIALIZED_DATA;
    for (i=0;i<NtHeader->FileHeader.NumberOfSections;i++) {

        IdataSection = IMAGE_FIRST_SECTION(NtHeader) + i;

        if ((IdataSection->Characteristics&flags) == flags ) {
            found = 1;
            break;
        }
    }

    if (!found) {
        status = STATUS_UNSUCCESSFUL;
        goto exit;
    }

    //
    // Try to find IofCompleteRequest 
    // pointer in this section
    //
    ImportStart = (PVOID*) ( ((PUCHAR)DosHeader) + IdataSection->VirtualAddress );
    ImportEnd = (PVOID*) ( ((PUCHAR)ImportStart)+IdataSection->SizeOfRawData );

    RtlInitUnicodeString(&us,L"IofCompleteRequest");
    IofCompleteRequestAdr = MmGetSystemRoutineAddress(&us);

    IofCompleteRequestImportPtr = NULL;

    while(ImportStart != ImportEnd) {

        if ( *ImportStart == IofCompleteRequestAdr) {

            IofCompleteRequestImportPtr = ImportStart;
            break;

        }

        ImportStart++;
    }

    if (NULL == IofCompleteRequestImportPtr) {
        status = STATUS_UNSUCCESSFUL;
        goto exit;
    }

    //
    // At this point we have (referenced) pointer to the driver object
    // and pointer to IofCompleteRequest IAT entry
    //

    //
    // Map import section as read-write. We're mapping system non-paged
    // memory, so we can afford a luxury not having a try/except. We also
    // assume that aligned sizeof(PVOID) occupies less then two pages :)
    //
    MmInitializeMdl(&ImportMdl.Mdl, IofCompleteRequestImportPtr, sizeof(PVOID));
    MmProbeAndLockPages(&ImportMdl.Mdl,KernelMode,IoModifyAccess);
    IofCompleteRequestImportPtrRw = MmGetSystemAddressForMdlSafe(&ImportMdl.Mdl,LowPagePriority);
    if (NULL==IofCompleteRequestImportPtrRw) {
        MmUnlockPages(&ImportMdl.Mdl);
        status = STATUS_UNSUCCESSFUL;
        goto exit;
    }

    //
    // We have a read-write pointer to IAT entry of IofCompleteRequest.
    // Put an address of our API there
    //
    InterlockedExchangePointer(IofCompleteRequestImportPtrRw, (PVOID) MyIofCompleteRequest);
    MmUnlockPages(&ImportMdl.Mdl);

    //
    // At this point Driver's IofCompleteRequest is patched so it is safe
    // to create a FASTIO table for it. From now on, no matter what, this 
    // function MUST return STATUS_SUCCESS so drver can be loaded resident 
    // in memory
    //
    ASSERT(status == STATUS_SUCCESS);

    //
    // Set up FastIo IOCTL dispatch
    //
    RtlZeroMemory(&FastIoDispatch,sizeof(FastIoDispatch));
    FastIoDispatch.SizeOfFastIoDispatch = sizeof(FAST_IO_DISPATCH);
    FastIoDispatch.FastIoDeviceControl = DispatchFastIoDeviceControl;

    InterlockedCompareExchangePointer( (PVOID*) &ParvdmDriver->FastIoDispatch, &FastIoDispatch , NULL );

exit:
    if (NULL!=ParvdmDriver) {
        ObDereferenceObject(ParvdmDriver);
    }
    return status;
}

//
// Create/Close - Do nothing
//
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

//
// Augumented IofCompleteRequest. It allows Irp pointer to be zero
// and if it recognizes specially marked IRP then it does nothing.
//
static
VOID
FASTCALL
MyIofCompleteRequest(
    IN PIRP Irp,
    IN CCHAR PriorityBoost
    )
{
    if (NULL==Irp) {
        return;
    }

    if (Irp->CancelRoutine == SPECIAL_CANCEL_MARK) {
        return;
    }

    IofCompleteRequest(Irp,PriorityBoost);
}

//
// Some defines related to parvdm IOCTLs
//

#define IOCTL_PP_WRITE_DATA         0x002C0004
#define IOCTL_PP_WRITE_CONTROL      0x002C0008
#define IOCTL_PP_READ_STATUS        0x002C000C

#define MAX_IOCTL_BUFFER    64


//
// This is FastIo IOCTL dispatch for PARVDM driver
//
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
    IRP                     FakeIrp;
    IO_STACK_LOCATION       FakeStackLocation;
    UCHAR                   FakeBuffer[MAX_IOCTL_BUFFER];

    //
    // Check that it is one of 3 known IOCTLS
    // (they all use METHOD_BUFFERED)
    //
    if ( (IoControlCode!=IOCTL_PP_WRITE_DATA) &&
         (IoControlCode!=IOCTL_PP_WRITE_CONTROL) &&
         (IoControlCode!=IOCTL_PP_READ_STATUS) )
    {
        return FALSE;
    }

    // 
    // Check that input/output sizes are valid
    //
    if ( (InputBufferLength>MAX_IOCTL_BUFFER) ||
         (OutputBufferLength>MAX_IOCTL_BUFFER) )
    {
        return FALSE;
    }

    //
    // Ok, it looks like we can process this request.
    // Probe all parameters and call the driver
    //

    __try {
        if (0!=InputBufferLength) {
            ProbeForRead(InputBuffer,InputBufferLength,1);
            RtlCopyMemory(FakeBuffer,InputBuffer,InputBufferLength);
        }
        if (0!=OutputBufferLength) {
            ProbeForWrite(OutputBuffer,OutputBufferLength,1);
        }
    }__except(EXCEPTION_EXECUTE_HANDLER) {
        IoStatus->Status = GetExceptionCode();
        IoStatus->Information = 0;
        return TRUE;
    }

    //
    // Construct the fake IRP. Set only few required fields
    //

    FakeIrp.CancelRoutine = SPECIAL_CANCEL_MARK;
    FakeIrp.Tail.Overlay.CurrentStackLocation = &FakeStackLocation;
    FakeIrp.AssociatedIrp.SystemBuffer = FakeBuffer;
    RtlZeroMemory(&FakeIrp.IoStatus,sizeof(FakeIrp.IoStatus));

    FakeStackLocation.Parameters.DeviceIoControl.IoControlCode = IoControlCode;
    FakeStackLocation.Parameters.DeviceIoControl.InputBufferLength = InputBufferLength;
    FakeStackLocation.Parameters.DeviceIoControl.OutputBufferLength = OutputBufferLength;

    //
    // Call driver IOCTL dispatch routine
    //
    IoStatus->Status = (DeviceObject->DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]) ( DeviceObject , &FakeIrp );

    //
    // Put result back
    //

    if (NT_SUCCESS(IoStatus->Status)) {

        IoStatus->Information = FakeIrp.IoStatus.Information;

        __try {
            if (0!=FakeIrp.IoStatus.Information) {
                RtlCopyMemory(OutputBuffer,FakeBuffer,FakeIrp.IoStatus.Information);
            }
        }__except(EXCEPTION_EXECUTE_HANDLER) {
            IoStatus->Status = GetExceptionCode();
            IoStatus->Information = 0;
            return TRUE;
        }

    }

    return TRUE;
}

