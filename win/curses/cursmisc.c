#include "curses.h"
#undef getch
#include "hack.h"
#include "wincurs.h"
#include "cursmisc.h"
#include "func_tab.h"
#include "dlb.h"

#include <ctype.h>

/* Misc. curses interface functions */

/* Private declarations */

static int curs_x = -1;
static int curs_y = -1;

static int NDECL(parse_escape_sequence);

/* Macros for Control and Alt keys */

#ifndef M
# ifndef NHSTDC
#  define M(c)		(0x80 | (c))
# else
#  define M(c)		((c) - 128)
# endif /* NHSTDC */
#endif
#ifndef C
#define C(c)		(0x1f & (c))
#endif


/* Read a character of input from the user */

int
curses_read_char()
{
    int ch, tmpch;

    ch = wgetch(stdscr);
    tmpch = ch;
    ch = curses_convert_keys(ch);

    if (ch == 0) {
        ch = '\033'; /* map NUL to ESC since nethack doesn't expect NUL */
    }

#if defined(ALT_0) && defined(ALT_9)    /* PDCurses, maybe others */
    if ((ch >= ALT_0) && (ch <= ALT_9)) {
        tmpch = (ch - ALT_0) + '0';
        ch = M(tmpch);
    }
#endif

#if defined(ALT_A) && defined(ALT_Z)    /* PDCurses, maybe others */
    if ((ch >= ALT_A) && (ch <= ALT_Z)) {
        tmpch = (ch - ALT_A) + 'a';
        ch = M(tmpch);
    }
#endif

#ifdef KEY_RESIZE
    /* Handle resize events via get_nh_event, not this code */
    if (ch == KEY_RESIZE) {
        ch = '\033'; /* NetHack doesn't know what to do with KEY_RESIZE */
    }
#endif

    if (counting && !isdigit(ch)) { /* Dismiss count window if necissary */
        curses_count_window(NULL);
        curses_refresh_nethack_windows();
    }

    return ch;
}

/* Turn on or off the specified color and / or attribute */

void
curses_toggle_color_attr(win, color, attr, onoff)
WINDOW *win;
int color;
int attr;
int onoff;
{
#ifdef TEXTCOLOR
    int curses_color;

    /* Map color disabled */
    if ((!iflags.wc_color) && (win == mapwin)) {
        return;
    }

    /* GUI color disabled */
    if (
#if 0 /*RLC unused*/
        (!iflags.wc2_guicolor) &&
#endif
        (win != mapwin)) {
        return;
    }

    if (color == 0) { /* make black fg visible */
#ifdef USE_DARKGRAY
        if (can_change_color() && (COLORS > 16)) {
            color = CURSES_DARK_GRAY - 1;
        } else { /* Use bold for a bright black */
            wattron(win, A_BOLD);
        }
#else
        color = CLR_BLUE;
#endif  /* USE_DARKGRAY */
    }
    curses_color = color + 1;
    if (COLORS < 16) {
        if (curses_color > 8)
            curses_color -= 8;
    }
    if (onoff == ON) {  /* Turn on color/attributes */
        if (color != NONE) {
            if ((color > 7) && (COLORS < 16)) {
                wattron(win, A_BOLD);
            }
            wattron(win, COLOR_PAIR(curses_color));
        }

        if (attr != NONE) {
            wattron(win, attr);
        }
    } else {            /* Turn off color/attributes */
        if (color != NONE) {
            if ((color > 7) && (COLORS < 16)) {
                wattroff(win, A_BOLD);
            }
#ifdef USE_DARKGRAY
            if ((color == 0) && (!can_change_color() ||
                                 (COLORS <= 16))) {
                wattroff(win, A_BOLD);
            }
#else
            if (iflags.use_inverse) {
                wattroff(win, A_REVERSE);
            }
#endif  /* DARKGRAY */
            wattroff(win, COLOR_PAIR(curses_color));
        }

        if (attr != NONE) {
            wattroff(win, attr);
        }
    }
#endif  /* TEXTCOLOR */
}


/* clean up and quit - taken from tty port */

void
curses_bail(mesg)
const char *mesg;
{
    clearlocks();
    curses_exit_nhwindows(mesg);
    terminate(EXIT_SUCCESS);
}


/* Return a winid for a new window of the given type */

winid
curses_get_wid(type)
int type;
{
    winid ret;
    static winid menu_wid = 20; /* Always even */
    static winid text_wid = 21; /* Always odd */

    switch (type) {
    case NHW_MESSAGE: {
        return MESSAGE_WIN;
        break;
    }
    case NHW_MAP: {
        return MAP_WIN;
        break;
    }
    case NHW_STATUS: {
        return STATUS_WIN;
        break;
    }
    case NHW_MENU: {
        ret = menu_wid;
        break;
    }
    case NHW_TEXT: {
        ret = text_wid;
        break;
    }
    default: {
        panic("curses_get_wid: unsupported window type");
        ret = -1;   /* Not reached */
    }
    }

    while (curses_window_exists(ret)) {
        ret += 2;
        if ((ret + 2) > 10000) {  /* Avoid "wid2k" problem */
            ret -= 9900;
        }
    }

    if (type == NHW_MENU) {
        menu_wid += 2;
    } else {
        text_wid += 2;
    }

    return ret;
}


/*
 * Allocate a copy of the given string.  If null, return a string of
 * zero length.
 *
 * This is taken from copy_of() in tty/wintty.c.
 */

char *
curses_copy_of(s)
const char *s;
{
    if (!s) s = "";
    return strcpy((char *) alloc((unsigned) (strlen(s) + 1)), s);
}


/* Determine the number of lines needed for a string for a dialog window
of the given width */

int
curses_num_lines(str, width)
const char *str;
int width;
{
    int last_space, count;
    int curline = 1;
    char substr[BUFSZ];
    char tmpstr[BUFSZ];

    strcpy(substr, str);

    while (strlen(substr) > width) {
        last_space = 0;

        for (count = 0; count <= width; count++) {
            if (substr[count] == ' ')
                last_space = count;

        }
        if (last_space == 0) {  /* No spaces found */
            last_space = count - 1;
        }
        for (count = (last_space + 1); count < strlen(substr); count++) {
            tmpstr[count - (last_space + 1)] = substr[count];
        }
        tmpstr[count - (last_space + 1)] = '\0';
        strcpy(substr, tmpstr);
        curline++;
    }

    return curline;
}


/* Break string into smaller lines to fit into a dialog window of the
given width */

char *
curses_break_str(str, width, line_num)
const char *str;
int width;
int line_num;
{
    str_context ctx = str_open_context("curses_break_str");
    int last_space, count;
    char *retstr;
    int curline = 0;
    int strsize = strlen(str);
    char *substr;
    char *curstr;
    char *tmpstr;

    substr = str_copy(str);
    curstr = str_alloc(strsize);
    tmpstr = str_alloc(strsize);

    while (curline < line_num) {
        if (strlen(substr) == 0 ) {
            break;
        }
        curline++;
        last_space = 0;
        for (count = 0; count <= width; count++) {
            if (substr[count] == ' ') {
                last_space = count;
            } else if (substr[count] == '\0') {
                last_space = count;
                break;
            }
        }
        if (last_space == 0) {  /* No spaces found */
            last_space = count - 1;
        }
        for (count = 0; count < last_space; count++) {
            curstr[count] = substr[count];
        }
        curstr[count] = '\0';
        if (substr[count] == '\0') {
            break;
        }
        for (count = (last_space + 1); count < strlen(substr); count++) {
            tmpstr[count - (last_space + 1)] = substr[count];
        }
        tmpstr[count - (last_space + 1)] = '\0';
        strcpy(substr, tmpstr);
    }

    if (curline < line_num) {
        retstr = NULL;
    } else {
        retstr = curses_copy_of(curstr);
    }

    str_close_context(ctx);
    return retstr;
}


/* Return the remaining portion of a string after hacking-off line_num lines */

char *
curses_str_remainder(str, width, line_num)
const char *str;
int width;
int line_num;
{
    str_context ctx = str_open_context("curses_str_remainder");
    int last_space, count;
    char *retstr;
    int curline = 0;
    int strsize = strlen(str);
    char *substr;
    char *curstr;
    char *tmpstr;

    substr = str_copy(str);
    curstr = str_alloc(strsize);
    tmpstr = str_alloc(strsize);

    while (curline < line_num) {
        if (strlen(substr) == 0 ) {
            break;
        }
        curline++;
        last_space = 0;
        for (count = 0; count <= width; count++) {
            if (substr[count] == ' ') {
                last_space = count;
            } else if (substr[count] == '\0') {
                last_space = count;
                break;
            }
        }
        if (last_space == 0) {  /* No spaces found */
            last_space = count - 1;
        }
        for (count = 0; count < last_space; count++) {
            curstr[count] = substr[count];
        }
        curstr[count] = '\0';
        if (substr[count] == '\0') {
            break;
        }
        for (count = (last_space + 1); count < strlen(substr); count++) {
            tmpstr[count - (last_space + 1)] = substr[count];
        }
        tmpstr[count - (last_space + 1)] = '\0';
        strcpy(substr, tmpstr);
    }

    if (curline < line_num) {
        retstr = NULL;
    } else {
        retstr = curses_copy_of(substr);
    }

    str_close_context(ctx);
    return retstr;
}


/* Determine if the given NetHack winid is a menu window */

boolean
curses_is_menu(wid)
winid wid;
{
    if ((wid > 19) && !(wid % 2)) { /* Even number */
        return TRUE;
    } else {
        return FALSE;
    }
}


/* Determine if the given NetHack winid is a text window */

boolean
curses_is_text(wid)
winid wid;
{
    if ((wid > 19) && (wid % 2)) { /* Odd number */
        return TRUE;
    } else {
        return FALSE;
    }
}


/* Replace certain characters with portable drawing characters if
cursesgraphics option is enabled */

int
curses_convert_glyph(ch)
int ch;
{
    if (SYMHANDLING(H_DEC)) {
        switch (ch) {
        case 0xE0: /* meta-\, diamond */
            ch = ACS_DIAMOND;
            break;

        case 0xE1: /* meta-a, solid block */
            ch = ACS_CKBOARD;
            break;

        case 0xe6: /* meta-f degree symbol */
            ch = ACS_DEGREE;
            break;

        case 0xE7: /* meta-g, plus-or-minus */
            ch = ACS_PLMINUS;
            break;

        case 0xEA: /* meta-j, bottom right */
            ch = ACS_LRCORNER;
            break;

        case 0xEB: /* meta-k, top right corner */
            ch = ACS_URCORNER;
            break;

        case 0xEC: /* meta-l, top left corner */
            ch = ACS_ULCORNER;
            break;

        case 0xED: /* meta-m, bottom left */
            ch = ACS_LLCORNER;
            break;

        case 0xEE: /* meta-n, cross */
            ch = ACS_PLUS;
            break;

        case 0xEF: /* meta-o, high horizontal line */
            ch = ACS_S1;
            break;

        case 0xF0: /* meta-p, scan line 3 */
            ch = ACS_S3;
            break;

        case 0xF1: /* meta-q, horizontal rule */
            ch = ACS_HLINE;
            break;

        case 0xF2: /* meta-r, scan line 7 */
            ch = ACS_S7;
            break;

        case 0xF3: /* meta-s, low horizontal line */
            ch = ACS_S9;
            break;

        case 0xF4: /* meta-t, T right */
            ch = ACS_RTEE;
            break;

        case 0xF5: /* meta-u, T left */
            ch = ACS_LTEE;
            break;

        case 0xF6: /* meta-v, T up */
            ch = ACS_TTEE;
            break;

        case 0xF7: /* meta-w, T down */
            ch = ACS_LTEE;
            break;

        case 0xF8: /* meta-x, vertical rule */
            ch = ACS_VLINE;
            break;

        case 0xF9: /* meta-y, greater-than-or-equals */
            ch = ACS_GEQUAL;
            break;

        case 0xFA: /* meta-z, less-than-or-equals */
            ch = ACS_LEQUAL;
            break;

        case 0xFB: /* meta-{, small pi */
            ch = ACS_PI;
            break;

        case 0xFC: /* meta-|, not equal */
            ch = ACS_NEQUAL;
            break;

        case 0xFD: /* meta-}, UK pound sign */
            ch = ACS_STERLING;
            break;

        case 0xFE: /* meta-~, centered dot */
            ch = ACS_BULLET;
            break;
        }
    } else if (SYMHANDLING(H_IBM)) {
        static const unsigned short cp437[] = {
            0x0020, 0x263A, 0x263B, 0x2665, 0x2666, 0x2663, 0x2660, 0x2022,
            0x25D8, 0x25CB, 0x25D9, 0x2642, 0x2640, 0x266A, 0x266B, 0x263C,
            0x25BA, 0x25C4, 0x2195, 0x203C, 0x00B6, 0x00A7, 0x25AC, 0x21A8,
            0x2191, 0x2193, 0x2192, 0x2190, 0x221F, 0x2194, 0x25B2, 0x25BC,
            0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,
            0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
            0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
            0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,
            0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
            0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f,
            0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
            0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f,
            0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
            0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f,
            0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
            0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x2302,
            0x00c7, 0x00fc, 0x00e9, 0x00e2, 0x00e4, 0x00e0, 0x00e5, 0x00e7,
            0x00ea, 0x00eb, 0x00e8, 0x00ef, 0x00ee, 0x00ec, 0x00c4, 0x00c5,
            0x00c9, 0x00e6, 0x00c6, 0x00f4, 0x00f6, 0x00f2, 0x00fb, 0x00f9,
            0x00ff, 0x00d6, 0x00dc, 0x00a2, 0x00a3, 0x00a5, 0x20a7, 0x0192,
            0x00e1, 0x00ed, 0x00f3, 0x00fa, 0x00f1, 0x00d1, 0x00aa, 0x00ba,
            0x00bf, 0x2310, 0x00ac, 0x00bd, 0x00bc, 0x00a1, 0x00ab, 0x00bb,
            0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556,
            0x2555, 0x2563, 0x2551, 0x2557, 0x255d, 0x255c, 0x255b, 0x2510,
            0x2514, 0x2534, 0x252c, 0x251c, 0x2500, 0x253c, 0x255e, 0x255f,
            0x255a, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256c, 0x2567,
            0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256b,
            0x256a, 0x2518, 0x250c, 0x2588, 0x2584, 0x258c, 0x2590, 0x2580,
            0x03b1, 0x00df, 0x0393, 0x03c0, 0x03a3, 0x03c3, 0x00b5, 0x03c4,
            0x03a6, 0x0398, 0x03a9, 0x03b4, 0x221e, 0x03c6, 0x03b5, 0x2229,
            0x2261, 0x00b1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00f7, 0x2248,
            0x00b0, 0x2219, 0x00b7, 0x221a, 0x207f, 0x00b2, 0x25a0, 0x00a0
        };
        ch = cp437[(unsigned char)ch];
    }

    return ch;
}


/* Move text cursor to specified coordinates in the given NetHack window */

void
curses_move_cursor(wid, x, y)
winid wid;
int x;
int y;
{
    int sx, sy, ex, ey;
    int xadj = 0;
    int yadj = 0;

#ifndef PDCURSES
    WINDOW *win = curses_get_nhwin(MAP_WIN);
#endif

    if (wid != MAP_WIN) {
        return;
    }

#ifdef PDCURSES
    /* PDCurses seems to not handle wmove correctly, so we use move and
    physical screen coordinates instead */
    curses_get_window_xy(wid, &xadj, &yadj);
#endif
    curs_x = x + xadj;
    curs_y = y + yadj;
    curses_map_borders(&sx, &sy, &ex, &ey, x, y);

    if (curses_window_has_border(wid)) {
        curs_x++;
        curs_y++;
    }

    if ((x >= sx) && (x <= ex) &&
            (y >= sy) && (y <= ey)) {
        curs_x -= sx;
        curs_y -= sy;
#ifdef PDCURSES
        move(curs_y, curs_x);
#else
        wmove(win, curs_y, curs_x);
#endif
    }
}


/* Perform actions that should be done every turn before nhgetch() */

void
curses_prehousekeeping()
{
#ifndef PDCURSES
    WINDOW *win = curses_get_nhwin(MAP_WIN);
#endif  /* PDCURSES */

    if ((curs_x > -1) && (curs_y > -1)) {
        curs_set(1);
#ifdef PDCURSES
        /* PDCurses seems to not handle wmove correctly, so we use move
        and physical screen coordinates instead */
        move(curs_y, curs_x);
#else
        wmove(win, curs_y, curs_x);
#endif  /* PDCURSES */
        curses_refresh_nhwin(MAP_WIN);
    }
}


/* Perform actions that should be done every turn after nhgetch() */

void
curses_posthousekeeping()
{
    curs_set(0);
    curses_decrement_highlight();
    curses_clear_unhighlight_message_window();
}


void
curses_view_file(filename, must_exist)
const char *filename;
boolean must_exist;
{
    winid wid;
    anything *identifier;
    char buf[BUFSZ];
    menu_item *selected = NULL;
    dlb *fp = dlb_fopen(filename, "r");

    if ((fp == NULL) && (must_exist)) {
        pline("Cannot open %s for reading!", filename);
    }

    if (fp == NULL) {
        return;
    }

    wid = curses_get_wid(NHW_MENU);
    curses_create_nhmenu(wid);
    identifier = malloc(sizeof(anything));
    identifier->a_void = NULL;

    while (dlb_fgets(buf, BUFSZ, fp) != NULL) {
        curses_add_menu(wid, NO_GLYPH, identifier, 0, 0, A_NORMAL, buf,
                        FALSE);
    }

    dlb_fclose(fp);
    curses_end_menu(wid, "");
    curses_select_menu(wid, PICK_NONE, &selected);
}


void
curses_rtrim(str)
char *str;
{
    char *s;

    for(s = str; *s != '\0'; ++s);
    for(--s; isspace(*s) && s > str; --s);
    if(s == str) *s = '\0';
    else *(++s) = '\0';
}


/* Read numbers until non-digit is encountered, and return number
in int form. */

int
curses_get_count(first_digit)
int first_digit;
{
    long current_count = first_digit;
    int current_char;

    current_char = curses_read_char();

    while (isdigit(current_char)) {
        current_count = (current_count * 10) + (current_char - '0');
        if (current_count > LARGEST_INT) {
            current_count = LARGEST_INT;
        }

        pline("Count: %ld", current_count);
        current_char = curses_read_char();
    }

    ungetch(current_char);

    if (current_char == '\033') {  /* Cancelled with escape */
        current_count = -1;
    }

    return current_count;
}


/* Convert the given NetHack text attributes into the format curses
understands, and return that format mask. */

int
curses_convert_attr(attr)
int attr;
{
    int curses_attr;

    switch (attr) {
    case ATR_NONE: {
        curses_attr = A_NORMAL;
        break;
    }
    case ATR_ULINE: {
        curses_attr = A_UNDERLINE;
        break;
    }
    case ATR_BOLD: {
        curses_attr = A_BOLD;
        break;
    }
    case ATR_BLINK: {
        curses_attr = A_BLINK;
        break;
    }
    case ATR_INVERSE: {
        curses_attr = A_REVERSE;
        break;
    }
    default: {
        curses_attr = A_NORMAL;
    }
    }

    return curses_attr;
}


/* Convert special keys into values that NetHack can understand.
Currently this is limited to arrow keys, but this may be expanded. */

int
curses_convert_keys(key)
int key;
{
    int ret = key;

    if (ret == '\033') {
        ret = parse_escape_sequence();
    }

    /* Handle arrow keys */
    switch (key) {
    case KEY_LEFT: {
        if (iflags.num_pad) {
            ret = '4';
        } else {
            ret = 'h';
        }
        break;
    }
    case KEY_RIGHT: {
        if (iflags.num_pad) {
            ret = '6';
        } else {
            ret = 'l';
        }
        break;
    }
    case KEY_UP: {
        if (iflags.num_pad) {
            ret = '8';
        } else {
            ret = 'k';
        }
        break;
    }
    case KEY_DOWN: {
        if (iflags.num_pad) {
            ret = '2';
        } else {
            ret = 'j';
        }
        break;
    }
#ifdef KEY_A1
    case KEY_A1: {
        if (iflags.num_pad) {
            ret = '7';
        } else {
            ret = 'y';
        }
        break;
    }
#endif  /* KEY_A1 */
#ifdef KEY_A3
    case KEY_A3: {
        if (iflags.num_pad) {
            ret = '9';
        } else {
            ret = 'u';
        }
        break;
    }
#endif  /* KEY_A3 */
#ifdef KEY_C1
    case KEY_C1: {
        if (iflags.num_pad) {
            ret = '1';
        } else {
            ret = 'b';
        }
        break;
    }
#endif  /* KEY_C1 */
#ifdef KEY_C3
    case KEY_C3: {
        if (iflags.num_pad) {
            ret = '3';
        } else {
            ret = 'n';
        }
        break;
    }
#endif  /* KEY_C3 */
#ifdef KEY_B2
    case KEY_B2: {
        if (iflags.num_pad) {
            ret = '5';
        } else {
            ret = 'g';
        }
        break;
    }
#endif  /* KEY_B2 */
    }

    return ret;
}


/* Process mouse events.  Mouse movement is processed until no further
mouse movement events are available.  Returns 0 for a mouse click
event, or the first non-mouse key event in the case of mouse
movement. */

int
curses_get_mouse(mousex, mousey, mod)
int *mousex;
int *mousey;
int *mod;
{
    int key = '\033';
#ifdef NCURSES_MOUSE_VERSION
    MEVENT event;

    if (getmouse(&event) == OK) {
        /* When the user clicks left mouse button */
        if(event.bstate & BUTTON1_CLICKED) {
            /* See if coords are in map window & convert coords */
            if (wmouse_trafo(mapwin, &event.y, &event.x, TRUE)) {
                key = 0;    /* Flag mouse click */
                *mousex = event.x;
                *mousey = event.y;

                if (curses_window_has_border(MAP_WIN)) {
                    (*mousex)--;
                    (*mousey)--;
                }

                *mod = CLICK_1;
            }
        }
    }
#endif  /* NCURSES_MOUSE_VERSION */

    return key;
}


static int
parse_escape_sequence()
{
#ifndef PDCURSES
    int ret;

    timeout(10);

    ret = wgetch(stdscr);

    if (ret != ERR) { /* Likely an escape sequence */
        if (((ret >= 'a') && (ret <= 'z')) ||
                ((ret >= '0') && (ret <= '9'))) {
            ret |= 0x80; /* Meta key support for most terminals */
        } else if (ret == 'O') { /* Numeric keypad */
            ret = wgetch(stdscr);
            if ((ret != ERR) && (ret >= 112) && (ret <= 121)) {
                ret = ret - 112 + '0';  /* Convert to number */
            } else {
                ret = '\033';    /* Escape */
            }
        }
    } else {
        ret = '\033';    /* Just an escape character */
    }

    timeout(-1);

    return ret;
#else
    return '\033';
#endif  /* !PDCURSES */
}


/* This is a kludge for the statuscolors patch which calls tty-specific
functions, which causes a compiler error if TTY_GRAPHICS is not
defined.  Adding stub functions to avoid this. */

#if defined(STATUS_COLORS) && !defined(TTY_GRAPHICS)
extern void term_start_color(color) int color; {}
extern void term_start_attr(attr) int attr; {}
extern void term_end_color() {}
extern void term_end_attr(int attr) {}
#endif  /* STATUS_COLORS && !TTY_GRAPGICS */
