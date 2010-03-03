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

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wf_event.h"
#include "wf_colour.h"
#include "wf_win.h"

extern LPCTSTR g_wnd_class_name;
extern HINSTANCE g_hInstance;

struct wf_bitmap
{
	HDC hdc;
	HBITMAP bitmap;
	HBITMAP org_bitmap;
};

static int
wf_convert_rop3(int rop3)
{
	switch (rop3)
	{
		case 0x00: /* 0 - 0 */
			return BLACKNESS;
		case 0x05: /* ~(P | D) - DPon */
			return NOTSRCERASE;
		case 0x0a: /* ~P & D - DPna */
			//XSetFunction(xfi->display, xfi->gc, GXandInverted);
			break;
		case 0x0f: /* ~P - Pn */
			return NOTSRCCOPY;
		case 0x11: /* ~(S | D) - DSon */
			return NOTSRCERASE;
		case 0x22: /* ~S & D - DSna */
			//XSetFunction(xfi->display, xfi->gc, GXandInverted);
			break;
		case 0x33: /* ~S - Sn */
			return NOTSRCCOPY;
		case 0x44: /* S & ~D - SDna */
			return SRCERASE;
		case 0x50: /* P & ~D - PDna */
			return SRCERASE;
		case 0x55: /* ~D - Dn */
			return DSTINVERT;
		case 0x5a: /* D ^ P - DPx */
			return SRCINVERT;
		case 0x5f: /* ~(P & D) - DPan */
			//XSetFunction(xfi->display, xfi->gc, GXnand);
			break;
		case 0x66: /* D ^ S - DSx */
			return SRCINVERT;
		case 0x77: /* ~(S & D) - DSan */
			//XSetFunction(xfi->display, xfi->gc, GXnand);
			break;
		case 0x88: /* D & S - DSa */
			return SRCAND;
		case 0x99: /* ~(S ^ D) - DSxn */
			//XSetFunction(xfi->display, xfi->gc, GXequiv);
			break;
		case 0xa0: /* P & D - DPa */
			return SRCAND;
		case 0xa5: /* ~(P ^ D) - PDxn */
			//XSetFunction(xfi->display, xfi->gc, GXequiv);
			break;
		case 0xaa: /* D - D */
			break;
		case 0xaf: /* ~P | D - DPno */
			return MERGEPAINT;
		case 0xbb: /* ~S | D - DSno */
			return MERGEPAINT;
		case 0xcc: /* S - S */
			return SRCCOPY;
		case 0xdd: /* S | ~D - SDno */
			//XSetFunction(xfi->display, xfi->gc, GXorReverse);
			break;
		case 0xee: /* D | S - DSo */
			return SRCPAINT;
		case 0xf0: /* P - P */
			return SRCCOPY;
		case 0xf5: /* P | ~D - PDno */
			//XSetFunction(xfi->display, xfi->gc, GXorReverse);
			break;
		case 0xfa: /* P | D - DPo */
			return SRCPAINT;
		case 0xff: /* 1 - 1 */
			return WHITENESS;
		default:
			printf("xf_set_rop3: unknonw rop3 %x\n", rop3);
			break;
	}
	return 0;
}

static void
wf_invalidate_region(wfInfo * wfi, int x1, int y1, int x2, int y2)
{
	if (wfi->update_pending)
	{
		wfi->update_rect.left = min(x1, wfi->update_rect.left);
		wfi->update_rect.top = min(y1, wfi->update_rect.top);
		wfi->update_rect.right = max(x2, wfi->update_rect.right);
		wfi->update_rect.bottom = max(y2, wfi->update_rect.bottom);
	}
	else
	{
		wfi->update_pending = 1;
		wfi->update_rect.left = x1;
		wfi->update_rect.top = y1;
		wfi->update_rect.right = x2;
		wfi->update_rect.bottom = y2;
	}
}

static void
l_ui_error(struct rdp_inst * inst, char * text)
{
	printf("ui_error: %s", text);
}

static void
l_ui_warning(struct rdp_inst * inst, char * text)
{
	printf("ui_warning: %s\n", text);
}

static void
l_ui_unimpl(struct rdp_inst * inst, char * text)
{
	printf("ui_unimpl: %s\n", text);
}

static void
l_ui_begin_update(struct rdp_inst * inst)
{
	printf("ui_begin_update\n");
}

static void
l_ui_end_update(struct rdp_inst * inst)
{
	printf("ui_end_update\n");
}

static void
l_ui_desktop_save(struct rdp_inst * inst, int offset, int x, int y,
	int cx, int cy)
{
	printf("ui_desktop_save:\n");
}

static void
l_ui_desktop_restore(struct rdp_inst * inst, int offset, int x, int y,
	int cx, int cy)
{
	printf("ui_desktop_restore:\n");
}

static RD_HGLYPH
l_ui_create_glyph(struct rdp_inst * inst, int width, int height, uint8 * data)
{
	wfInfo * wfi;

	wfi = GET_WFI(inst);
	return (RD_HGLYPH)wf_glyph_convert(wfi, width, height, data);
}

static void
l_ui_destroy_glyph(struct rdp_inst * inst, RD_HGLYPH glyph)
{
	free(glyph);
}

static RD_HBITMAP
l_ui_create_bitmap(struct rdp_inst * inst, int width, int height, uint8 * data)
{
	wfInfo * wfi;
	struct wf_bitmap * bm;
	BITMAPINFO bmi = {0};
	uint8 * cdata;
	HDC hdc;

	wfi = GET_WFI(inst);
	bm = (struct wf_bitmap *) malloc(sizeof(struct wf_bitmap));
	hdc = GetDC(NULL);
	bm->hdc = CreateCompatibleDC(hdc);
	bm->bitmap = CreateCompatibleBitmap(hdc, width, height);
	ReleaseDC(NULL, hdc);
	bm->org_bitmap = (HBITMAP)SelectObject(bm->hdc, bm->bitmap);
	bmi.bmiHeader.biSize = sizeof(BITMAPINFO);
	bmi.bmiHeader.biWidth = width;
	bmi.bmiHeader.biHeight = height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biBitCount = 32;
	cdata = wf_image_convert(wfi, inst->settings, width, height, data);
	SetDIBits(bm->hdc, bm->bitmap, 0, height, cdata, &bmi, 0);
	if (cdata != data)
	{
		free(cdata);
	}

	return (RD_HBITMAP) bm;
}

static void
l_ui_destroy_bitmap(struct rdp_inst * inst, RD_HBITMAP bmp)
{
	struct wf_bitmap * bm;

	bm = (struct wf_bitmap *) bmp;
	SelectObject(bm->hdc, bm->org_bitmap);
	DeleteObject(bm->bitmap);
	DeleteDC(bm->hdc);
	free(bm);
}

static void
l_ui_paint_bitmap(struct rdp_inst * inst, int x, int y, int cx, int cy, int width,
	int height, uint8 * data)
{
	wfInfo * wfi;
	struct wf_bitmap * bm;

	wfi = GET_WFI(inst);
	bm = (struct wf_bitmap *) l_ui_create_bitmap(inst, width, height, data);
	BitBlt(wfi->hdc, x, y, cx, cy, bm->hdc, 0, 0, SRCCOPY);
	l_ui_destroy_bitmap(inst, (RD_HBITMAP)bm);
	wf_invalidate_region(wfi, x, y, x + cx, y + cy);
}

static void
l_ui_line(struct rdp_inst * inst, uint8 opcode, int startx, int starty, int endx,
	int endy, RD_PEN * pen)
{
	wfInfo * wfi;

	wfi = GET_WFI(inst);
	//TODO
}

static void
l_ui_rect(struct rdp_inst * inst, int x, int y, int cx, int cy, int colour)
{
	wfInfo * wfi;
	RECT rect;
	HBRUSH brush;

	wfi = GET_WFI(inst);
	colour = wf_colour_convert(wfi, inst->settings, colour);
	//printf("ui_rect %i %i %i %i %i\n", x, y, cx, cy, colour);
	rect.left = x;
	rect.top = y;
	rect.right = x + cx;
	rect.bottom = y + cy;
	brush = CreateSolidBrush(colour);
	FillRect(wfi->hdc, &rect, brush);
	DeleteObject(brush);
	wf_invalidate_region(wfi, rect.left, rect.top, rect.right, rect.bottom);
}

static void
l_ui_polygon(struct rdp_inst * inst, uint8 opcode, uint8 fillmode, RD_POINT * point,
	int npoints, RD_BRUSH * brush, int bgcolour, int fgcolour)
{
	printf("ui_polygon:\n");
}

static void
l_ui_polyline(struct rdp_inst * inst, uint8 opcode, RD_POINT * points, int npoints,
	RD_PEN * pen)
{
	wfInfo * wfi;

	wfi = GET_WFI(inst);
	//TODO
}

static void
l_ui_ellipse(struct rdp_inst * inst, uint8 opcode, uint8 fillmode, int x, int y,
	int cx, int cy, RD_BRUSH * brush, int bgcolour, int fgcolour)
{
	printf("ui_ellipse:\n");
}

static void
l_ui_start_draw_glyphs(struct rdp_inst * inst, int bgcolour, int fgcolour)
{
	wfInfo * wfi;

	wfi = GET_WFI(inst);
	wfi->bgcolour = wf_colour_convert(wfi, inst->settings, bgcolour);
	wfi->fgcolour = wf_colour_convert(wfi, inst->settings, fgcolour);
}

static void
l_ui_draw_glyph(struct rdp_inst * inst, int x, int y, int cx, int cy,
	RD_HGLYPH glyph)
{
	wfInfo * wfi;
	uint8 * data;
	HDC hdc;
	HBITMAP bitmap;
	HBITMAP org_bitmap;
	BITMAPINFO bmi = {0};

	wfi = GET_WFI(inst);
	data = wf_glyph_generate(wfi, cx, cy, (uint8 *) glyph);
	hdc = CreateCompatibleDC(wfi->hdc);
	bitmap = CreateCompatibleBitmap(wfi->hdc, cx, cy);
	org_bitmap = (HBITMAP)SelectObject(hdc, bitmap);
	bmi.bmiHeader.biSize = sizeof(BITMAPINFO);
	bmi.bmiHeader.biWidth = cx;
	bmi.bmiHeader.biHeight = cy;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biBitCount = 32;
	SetDIBits(hdc, bitmap, 0, cy, data, &bmi, 0);
	BitBlt(wfi->hdc, x, y, cx, cy, hdc, 0, 0, SRCCOPY);

	free(data);
	SelectObject(hdc, org_bitmap);
	DeleteObject(bitmap);
	DeleteDC(hdc);

	wf_invalidate_region(wfi, x, y, x + cx, y + cy);
}

static void
l_ui_end_draw_glyphs(struct rdp_inst * inst, int x, int y, int cx, int cy)
{
}

static uint32
l_ui_get_toggle_keys_state(struct rdp_inst * inst)
{
	wfInfo * wfi;

	wfi = GET_WFI(inst);
	//TODO
	return 0;
}

static void
l_ui_bell(struct rdp_inst * inst)
{
	printf("ui_bell:\n");
}

static void
l_ui_destblt(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy)
{
	wfInfo * wfi;

	wfi = GET_WFI(inst);
	//TODO
}

static void
l_ui_patblt(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy,
	RD_BRUSH * brush, int bgcolour, int fgcolour)
{
	wfInfo * wfi;

	wfi = GET_WFI(inst);
	//TODO
}

static void
l_ui_screenblt(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy,
	int srcx, int srcy)
{
	wfInfo * wfi;

	wfi = GET_WFI(inst);
	//TODO
}

static void
l_ui_memblt(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy,
	RD_HBITMAP src, int srcx, int srcy)
{
	wfInfo * wfi;
	struct wf_bitmap * bm;

	wfi = GET_WFI(inst);
	bm = (struct wf_bitmap *) src;
	//printf("ui_memblt %i %i %i %i %i %i %i\n", x, y, cx, cy, srcx, srcy, opcode);
	BitBlt(wfi->hdc, x, y, cx, cy, bm->hdc, srcx, srcy, wf_convert_rop3(opcode));
	wf_invalidate_region(wfi, x, y, x + cx, y + cy);
}

static void
l_ui_triblt(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy,
	RD_HBITMAP src, int srcx, int srcy, RD_BRUSH * brush, int bgcolour, int fgcolour)
{
	wfInfo * wfi;

	wfi = GET_WFI(inst);
	//TODO
}

static int
l_ui_select(struct rdp_inst * inst, int rdp_socket)
{
	//printf("ui_select: inst %p\n", inst);
	return 1;
}

static void
l_ui_set_clip(struct rdp_inst * inst, int x, int y, int cx, int cy)
{
	wfInfo * wfi;
	HRGN hrgn;

	wfi = GET_WFI(inst);
	printf("ui_set_clip %i %i %i %i\n", x, y, cx, cy);
	hrgn = CreateRectRgn(x, y, x + cx, y + cy);
	SelectClipRgn(wfi->hdc, hrgn);
	DeleteObject(hrgn);
}

static void
l_ui_reset_clip(struct rdp_inst * inst)
{
	wfInfo * wfi;

	wfi = GET_WFI(inst);
	printf("ui_reset_clip\n");
	SelectClipRgn(wfi->hdc, NULL);
}

static void
l_ui_resize_window(struct rdp_inst * inst)
{
	printf("ui_resize_window:\n");
}

static void
l_ui_set_cursor(struct rdp_inst * inst, RD_HCURSOR cursor)
{
	wfInfo * wfi;

	wfi = GET_WFI(inst);
	//TODO
}

static void
l_ui_destroy_cursor(struct rdp_inst * inst, RD_HCURSOR cursor)
{
	wfInfo * wfi;

	wfi = GET_WFI(inst);
	//TODO
}

static RD_HCURSOR
l_ui_create_cursor(struct rdp_inst * inst, uint32 x, uint32 y,
	int width, int height, uint8 * andmask, uint8 * xormask, int bpp)
{
	wfInfo * wfi;

	wfi = GET_WFI(inst);
	//TODO
	return NULL;
}

static void
l_ui_set_null_cursor(struct rdp_inst * inst)
{
	wfInfo * wfi;

	wfi = GET_WFI(inst);
	//TODO
}

static void
l_ui_set_default_cursor(struct rdp_inst * inst)
{
	wfInfo * wfi;

	wfi = GET_WFI(inst);
	//TODO
}

static RD_HCOLOURMAP
l_ui_create_colourmap(struct rdp_inst * inst, RD_COLOURMAP * colours)
{
	wfInfo * wfi;

	wfi = GET_WFI(inst);
	return wf_create_colourmap(wfi, inst->settings, colours);
}

static void
l_ui_move_pointer(struct rdp_inst * inst, int x, int y)
{
	wfInfo * wfi;

	wfi = GET_WFI(inst);
	//TODO
}

static void
l_ui_set_colourmap(struct rdp_inst * inst, RD_HCOLOURMAP map)
{
	wfInfo * wfi;

	wfi = GET_WFI(inst);
	wf_set_colourmap(wfi, inst->settings, map);
}

static RD_HBITMAP
l_ui_create_surface(struct rdp_inst * inst, int width, int height, RD_HBITMAP old_surface)
{
	wfInfo * wfi;

	wfi = GET_WFI(inst);
	//TODO
	return NULL;
}

static void
l_ui_set_surface(struct rdp_inst * inst, RD_HBITMAP surface)
{
	wfInfo * wfi;

	wfi = GET_WFI(inst);
	//TODO
}

static void
l_ui_destroy_surface(struct rdp_inst * inst, RD_HBITMAP surface)
{
	wfInfo * wfi;

	wfi = GET_WFI(inst);
	//TODO
}

static void
l_ui_channel_data(struct rdp_inst * inst, int chan_id, char * data, int data_size,
	int flags, int total_size)
{
	//TODO
}

static int
wf_assign_callbacks(rdpInst * inst)
{
	inst->ui_error = l_ui_error;
	inst->ui_warning = l_ui_warning;
	inst->ui_unimpl = l_ui_unimpl;
	inst->ui_begin_update = l_ui_begin_update;
	inst->ui_end_update = l_ui_end_update;
	inst->ui_desktop_save = l_ui_desktop_save;
	inst->ui_desktop_restore = l_ui_desktop_restore;
	inst->ui_create_bitmap = l_ui_create_bitmap;
	inst->ui_paint_bitmap = l_ui_paint_bitmap;
	inst->ui_destroy_bitmap = l_ui_destroy_bitmap;
	inst->ui_line = l_ui_line;
	inst->ui_rect = l_ui_rect;
	inst->ui_polygon = l_ui_polygon;
	inst->ui_polyline = l_ui_polyline;
	inst->ui_ellipse = l_ui_ellipse;
	inst->ui_start_draw_glyphs = l_ui_start_draw_glyphs;
	inst->ui_draw_glyph = l_ui_draw_glyph;
	inst->ui_end_draw_glyphs = l_ui_end_draw_glyphs;
	inst->ui_get_toggle_keys_state = l_ui_get_toggle_keys_state;
	inst->ui_bell = l_ui_bell;
	inst->ui_destblt = l_ui_destblt;
	inst->ui_patblt = l_ui_patblt;
	inst->ui_screenblt = l_ui_screenblt;
	inst->ui_memblt = l_ui_memblt;
	inst->ui_triblt = l_ui_triblt;
	inst->ui_create_glyph = l_ui_create_glyph;
	inst->ui_destroy_glyph = l_ui_destroy_glyph;
	inst->ui_select = l_ui_select;
	inst->ui_set_clip = l_ui_set_clip;
	inst->ui_reset_clip = l_ui_reset_clip;
	inst->ui_resize_window = l_ui_resize_window;
	inst->ui_set_cursor = l_ui_set_cursor;
	inst->ui_destroy_cursor = l_ui_destroy_cursor;
	inst->ui_create_cursor = l_ui_create_cursor;
	inst->ui_set_null_cursor = l_ui_set_null_cursor;
	inst->ui_set_default_cursor = l_ui_set_default_cursor;
	inst->ui_create_colourmap = l_ui_create_colourmap;
	inst->ui_move_pointer = l_ui_move_pointer;
	inst->ui_set_colourmap = l_ui_set_colourmap;
	inst->ui_create_surface = l_ui_create_surface;
	inst->ui_set_surface = l_ui_set_surface;
	inst->ui_destroy_surface = l_ui_destroy_surface;
	inst->ui_channel_data = l_ui_channel_data;
	return 0;
}

int
wf_pre_connect(rdpInst * inst, HWND hwnd)
{
	wfInfo * wfi;

	wf_assign_callbacks(inst);
	wfi = (wfInfo *) malloc(sizeof(wfInfo));
	SET_WFI(inst, wfi);
	memset(wfi, 0, sizeof(wfInfo));

	wfi->hwnd = hwnd;
	wfi->inst = inst;
	SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)wfi);

	return 0;
}

int
wf_post_connect(rdpInst * inst)
{
	wfInfo * wfi;
	int width;
	int height;
	HDC hdc;

	wfi = GET_WFI(inst);
	width = inst->settings->width;
	height = inst->settings->height;

	hdc = GetDC(NULL);
	wfi->hdc = CreateCompatibleDC(hdc);
	wfi->bitmap = CreateCompatibleBitmap(hdc, width, height);
	wfi->org_bitmap = (HBITMAP)SelectObject(wfi->hdc, wfi->bitmap);
	BitBlt(wfi->hdc, 0, 0, width, height, NULL, 0, 0, BLACKNESS);

	ReleaseDC(NULL, hdc);

	MoveWindow(wfi->hwnd, 0, 0, width, height, FALSE);
	ShowWindow(wfi->hwnd, SW_SHOWNORMAL);
	UpdateWindow(wfi->hwnd);

	return 0;
}

int
wf_update_window(rdpInst * inst)
{
	wfInfo * wfi;

	wfi = GET_WFI(inst);
	if (wfi->update_pending)
	{
		InvalidateRect(wfi->hwnd, &wfi->update_rect, FALSE);
		wfi->update_pending = 0;
		return 1;
	}
	return 0;
}

void
wf_uninit(rdpInst * inst)
{
	wfInfo * wfi;

	wfi = GET_WFI(inst);
	CloseWindow(wfi->hwnd);
	if (wfi->hdc)
	{
		SelectObject(wfi->hdc, wfi->org_bitmap);
		DeleteObject(wfi->bitmap);
		DeleteDC(wfi->hdc);
	}
	
	free(wfi);
}
