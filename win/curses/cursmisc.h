#ifndef CURSMISC_H
#define CURSMISC_H

/* Global declarations */

int NDECL(curses_read_char);

void FDECL(curses_toggle_color_attr, (WINDOW *win, int color, int attr, int onoff));

void FDECL(curses_bail, (const char *mesg));

winid FDECL(curses_get_wid, (int type));

char *FDECL(curses_copy_of, (const char *s));

int FDECL(curses_num_lines, (const char *str, int width));

char *FDECL(curses_break_str, (const char *str, int width, int line_num));

char *FDECL(curses_str_remainder, (const char *str, int width, int line_num));

boolean FDECL(curses_is_menu, (winid wid));

boolean FDECL(curses_is_text, (winid wid));

int FDECL(curses_convert_glyph, (int ch));

void FDECL(curses_move_cursor, (winid wid, int x, int y));

void NDECL(curses_prehousekeeping);

void NDECL(curses_posthousekeeping);

void FDECL(curses_view_file, (const char *filename, BOOLEAN_P must_exist));

void FDECL(curses_rtrim, (char *str));

int FDECL(curses_get_count, (int first_digit));

int FDECL(curses_convert_attr, (int attr));

int FDECL(curses_read_attrs, (char *attrs));

int FDECL(curses_convert_keys, (int key));

int FDECL(curses_get_mouse, (int *mousex, int *mousey, int *mod));

#endif  /* CURSMISC_H */
