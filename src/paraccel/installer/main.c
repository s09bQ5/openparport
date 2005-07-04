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
 *   Paraccel driver installer
 *
 */
#include <windows.h>
#include <stdio.h>

void PrintError()
{
    PSTR Msg=NULL;
    DWORD dwErr=GetLastError();

    if (0==FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        dwErr,
        0,
        (PSTR)&Msg,
        1,
        NULL))
    {
        puts("Unknown error occured");
        return;
    }

    fprintf(stderr,"Error occured: %s\n",Msg);

    LocalFree(Msg);

    return;
}

int __cdecl main(int argc,char** argv)
{
    SC_HANDLE hScm,hSvc;
    WCHAR   DstName[MAX_PATH+4];

    ExpandEnvironmentStringsW(L"%WINDIR%\\System32\\DRIVERS\\paraccel.sys",DstName,MAX_PATH);

    if (0==CopyFileW(L".\\paraccel.sys",DstName,FALSE)) {
        PrintError();
        return 1;
    }

    hScm = OpenSCManagerW(NULL,NULL,SC_MANAGER_CREATE_SERVICE);
    if (NULL==hScm) {
        PrintError();
        return 2;
    }

    hSvc = CreateServiceW(hScm,
        L"paraccel",
        L"Parallel port low-level access accelerator",
        SERVICE_START|SERVICE_STOP|STANDARD_RIGHTS_REQUIRED,
        SERVICE_KERNEL_DRIVER,
        SERVICE_AUTO_START,
        SERVICE_ERROR_NORMAL,
        L"System32\\DRIVERS\\paraccel.sys",
        NULL,
        NULL,
        L"parvdm\0\0",
        NULL,
        L"");

    if (NULL==hSvc) {
        PrintError();
        return 3;
    }

    if (0==StartService(hSvc,0,NULL)) {
        PrintError();
        return 4;
    }

    CloseServiceHandle(hSvc);
    CloseServiceHandle(hScm);

    puts("Driver paraccel.sys was installed successfully");
    return 0;
}

