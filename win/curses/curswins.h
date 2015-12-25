#ifndef CURSWIN_H
#define CURSWIN_H


/* Global declarations */

WINDOW *FDECL(curses_create_window, (int width, int height, orient orientation));

void FDECL(curses_destroy_win, (WINDOW *win));

void NDECL(curses_refresh_nethack_windows);

WINDOW *FDECL(curses_get_nhwin, (winid wid));

void FDECL(curses_add_nhwin, (winid wid, int height, int width, int y, int x,
 orient orientation, BOOLEAN_P border));

void FDECL(curses_add_wid, (winid wid));

void FDECL(curses_refresh_nhwin, (winid wid));

void FDECL(curses_del_nhwin, (winid wid));

void FDECL(curses_del_wid, (winid wid));

void FDECL(curses_putch, (winid wid, int x, int y, int ch, int color, int attrs));

void FDECL(curses_get_window_xy, (winid wid, int *x, int *y));

boolean FDECL(curses_window_has_border, (winid wid));

boolean FDECL(curses_window_exists, (winid wid));

int FDECL(curses_get_window_orientation, (winid wid));

void FDECL(curses_puts, (winid wid, int attr, const char *text));

void FDECL(curses_clear_nhwin, (winid wid));

void FDECL(curses_draw_map, (int sx, int sy, int ex, int ey));

boolean FDECL(curses_map_borders, (int *sx, int *sy, int *ex, int *ey, int ux,
 int uy));


#endif  /* CURSWIN_H */
