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
 *   /dev/ppdev user-mode emulation code
 *
 */
#include <windows.h>
#include <ppdev/ppdev.h>
#include <ppapi.h>

typedef struct _PP_handle
{
    volatile LONG   Busy;
    HANDLE          Device;
    UCHAR           LastCtrl;
} PP_handle;

#define PP_MAX_HANDLES 10

static PP_handle    handles[PP_MAX_HANDLES]={ {0,NULL,0 } };


int PP_open(const char *filename,int oflag,...)
{
    PP_handle   *phandle=NULL;
    int i,j;

    for (j=0;j<2;j++) {
        for (i=0;i<PP_MAX_HANDLES;i++) {

            if (0!=handles[i].Busy) continue;

            if (0==InterlockedCompareExchange((LPLONG)&handles[i].Busy,1,0)) {
                phandle = & handles[i];
                goto exitloop;
            }

        }
    }
    exitloop:

    if (NULL==phandle) {
        return -1;
    }

    //
    // We care only about last digit in the file name.
    //
    if ('1'==filename[strlen(filename)-1]) {
        i=1;
    } else {
        i=0;
    }

    phandle->Device = PPOpen(i);

    if (NULL==phandle->Device) {
        phandle->Busy=0;
        return -1;
    }

    phandle->LastCtrl = 0;

    return( 10 + (int)(phandle - handles) );
}

int PP_close(int handle)
{
    PP_handle   *phandle;

    if ( (handle<10) || (handle >= (10+PP_MAX_HANDLES)) ) {
        return -1;
    }

    phandle = handles + (handle-10);

    if (0==phandle->Busy) {
        return -1;
    }

    CloseHandle(phandle->Device);
    phandle->Busy=0;

    return 0;
}

static int PPReadByte(PP_handle *phandle,unsigned char *data)
{
    if (0==PPRead(phandle->Device,data)) {
        return -1;
    }

    return 0;
}

static int PPWriteByte(PP_handle *phandle,int offset,unsigned char data)
{
    if (0==PPWrite(phandle->Device,(UCHAR)offset,data)) {
        return -1;
    }

    return 0;
}


int PP_ioctl(int handle,int code,...)
{
    PP_handle   *phandle;
    unsigned char *p3;
    va_list     lst;
    int         err;
    struct ppdev_frob_struct    *frob;
    unsigned char   newctl;

    if ( (handle<10) || (handle >= (10+PP_MAX_HANDLES)) ) {
        return -1;
    }

    phandle = handles + (handle-10);

    if (0==phandle->Busy) {
        return -1;
    }

    va_start(lst,code);
    p3 = va_arg(lst,unsigned char*);
    va_end(lst);

    switch(code) {

        //
        // Unsupported iocts
        //

        case PPSETMODE :
        case PPWSTATUS :
        case PPRECONTROL :
        case PPWECONTROL :
        case PPNEGOT:
        case PPGETTIME:
        case PPSETTIME:
        case PPGETFLAGS:
        case PPSETFLAGS:
        case PPRFIFO:
        case PPWFIFO:
        case PPWCTLONIRQ:
        case PPCLRIRQ:
        default:
            return -1;

        //
        // Always "ok" ioctls
        //


        case PPCLAIM:
        case PPRELEASE:
        case PPYIELD:
        case PPEXCL:
            return 0;

        //
        //  Actual emulated IOCTLs
        //
        case PPRSTATUS:
            return PPReadByte(phandle,p3);

        case PPRCONTROL:
            *p3 = phandle->LastCtrl;
            return 0;

        case PPWCONTROL:
            err = PPWriteByte(phandle,2,*p3);
            if (0==err) {
                phandle->LastCtrl = *p3;
            }
            return err;

        case PPFCONTROL:
            frob = (struct ppdev_frob_struct    *) p3;
            newctl = (phandle->LastCtrl & (~frob->mask)) | frob->val;

            err = PPWriteByte(phandle,2,newctl);
            if (0==err) {
                phandle->LastCtrl = newctl;
            }
            return err;

        case PPRDATA :
            return 0;

        case PPWDATA:
            return PPWriteByte(phandle,0,*p3); 

    }

}
