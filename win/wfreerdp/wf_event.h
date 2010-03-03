/*
   Copyright (c) 2009 Jay Sorg
   Copyright (c) 2010 Vic Lee

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
*/

#ifndef __WF_EVENT_H
#define __WF_EVENT_H

#include "freerdp.h"

#define SET_WFI(_inst, _wfi) (_inst)->param1 = _wfi
#define GET_WFI(_inst) ((wfInfo *) ((_inst)->param1))

struct wf_info
{
	HWND hwnd;
	rdpInst * inst;
	HDC hdc;
	HBITMAP org_bitmap;
	HBITMAP bitmap;
	uint8 * colourmap;
	int bgcolour;
	int fgcolour;
	RECT update_rect;
	int update_pending;
};
typedef struct wf_info wfInfo;

LRESULT CALLBACK
wf_event_proc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

#endif
