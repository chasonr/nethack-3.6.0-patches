/* NetHack 3.6	pckeys.c	$NHDT-Date: 1432512792 2015/05/25 00:13:12 $  $NHDT-Branch: master $:$NHDT-Revision: 1.10 $ */
/* Copyright (c) NetHack PC Development Team 1996                 */
/* NetHack may be freely redistributed.  See license for details. */

/*
 *  MSDOS tile-specific key handling.
 */

#include "hack.h"

#ifdef MSDOS
#ifdef USE_TILES
#include "wintty.h"
#include "pcvideo.h"

boolean FDECL(pckeys, (unsigned char, unsigned char));
static void FDECL(userpan, (enum vga_pan_direction));
static void FDECL(overview, (BOOLEAN_P));
static void FDECL(traditional, (BOOLEAN_P));
static void NDECL(refresh);

extern struct WinDesc *wins[MAXWIN]; /* from wintty.c */
extern boolean inmap;                /* from video.c */

#define SHIFT (0x1 | 0x2)
#define CTRL 0x4
#define ALT 0x8

/*
 * Check for special interface manipulation keys.
 * Returns TRUE if the scan code triggered something.
 *
 */
boolean
pckeys(scancode, shift)
unsigned char scancode;
unsigned char shift;
{
    boolean opening_dialog;

    opening_dialog = pl_character[0] ? FALSE : TRUE;
#ifdef SIMULATE_CURSOR
    switch (scancode) {
    case 0x3d: /* F3 = toggle cursor type */
        HideCursor();
        cursor_type += 1;
        if (cursor_type >= NUM_CURSOR_TYPES)
            cursor_type = 0;
        DrawCursor();
        break;
#endif
    case 0x74: /* Control-right_arrow = scroll horizontal to right */
        if ((shift & CTRL) && iflags.tile_view && !opening_dialog)
            userpan(pan_right);
        break;

    case 0x73: /* Control-left_arrow = scroll horizontal to left */
        if ((shift & CTRL) && iflags.tile_view && !opening_dialog)
            userpan(pan_left);
        break;
    /* Dosbox reports control-up and down arrows, but VirtualBox and QEMU
       don't; accept home, end, page up and page down also */
    case 0x77: /* control-home */
    case 0x8D: /* control-up_arrow */
    case 0x84: /* control-page_up */
        if ((shift & CTRL) && iflags.tile_view && !opening_dialog)
            userpan(pan_up);
        break;
    case 0x75: /* control-end */
    case 0x91: /* control-down_arrow */
    case 0x76: /* control-page_down */
        if ((shift & CTRL) && iflags.tile_view && !opening_dialog)
            userpan(pan_down);
        break;
    case 0x3E: /* F4 = toggle overview mode */
        if (iflags.tile_view && !opening_dialog && !Is_rogue_level(&u.uz)) {
            iflags.traditional_view = FALSE;
            overview(iflags.over_view ? FALSE : TRUE);
            refresh();
        }
        break;
    case 0x3F: /* F5 = toggle traditional mode */
        if (iflags.tile_view && !opening_dialog && !Is_rogue_level(&u.uz)) {
            iflags.over_view = FALSE;
            traditional(iflags.traditional_view ? FALSE : TRUE);
            refresh();
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

static void
userpan(pan)
enum vga_pan_direction pan;
{
#ifdef SCREEN_VGA
    if (iflags.usevga)
        vga_userpan(pan);
#endif
#ifdef SCREEN_VESA
    if (iflags.usevesa)
        vesa_userpan(pan);
#endif
}

static void
overview(on)
boolean on;
{
#ifdef SCREEN_VGA
    if (iflags.usevga)
        vga_overview(on);
#endif
#ifdef SCREEN_VESA
    if (iflags.usevesa)
        vesa_overview(on);
#endif
}

static void
traditional(on)
boolean on;
{
#ifdef SCREEN_VGA
    if (iflags.usevga)
        vga_traditional(on);
#endif
#ifdef SCREEN_VESA
    if (iflags.usevesa)
        vesa_traditional(on);
#endif
}

static void
refresh()
{
#ifdef SCREEN_VGA
    if (iflags.usevga)
        vga_refresh();
#endif
#ifdef SCREEN_VESA
    if (iflags.usevesa)
        vesa_refresh();
#endif
}
#endif /* USE_TILES */
#endif /* MSDOS */

/*pckeys.c*/
