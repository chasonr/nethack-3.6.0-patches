#ifndef WINCURS_H
#define WINCURS_H

/* Global declarations for curses interface */

int term_rows, term_cols; /* size of underlying terminal */

WINDOW *base_term;    /* underlying terminal window */

WINDOW *mapwin, *statuswin, *messagewin;    /* Main windows */

boolean counting;   /* Count window is active */


#define TEXTCOLOR   /* Allow color */
#define NHW_END 19
#define OFF 0
#define ON 1
#define NONE -1
#define KEY_ESC 0x1b
#define DIALOG_BORDER_COLOR CLR_MAGENTA
#define SCROLLBAR_COLOR CLR_MAGENTA
#define SCROLLBAR_BACK_COLOR CLR_BLACK
#define HIGHLIGHT_COLOR CLR_WHITE
#define MORECOLOR CLR_ORANGE
#define STAT_UP_COLOR CLR_GREEN
#define STAT_DOWN_COLOR CLR_RED
#define MESSAGE_WIN 1
#define STATUS_WIN  2
#define MAP_WIN     3
#define NHWIN_MAX   4
#define MESG_HISTORY_MAX   200
#if !defined(__APPLE__) || !defined(NCURSES_VERSION)
# define USE_DARKGRAY /* Allow "bright" black; delete if not visible */
#endif	/* !__APPLE__ && !PDCURSES */
#define CURSES_DARK_GRAY    17
#define MAP_SCROLLBARS
#ifdef PDCURSES
# define getmouse nc_getmouse
# ifndef NCURSES_MOUSE_VERSION
#  define NCURSES_MOUSE_VERSION
# endif
#endif


typedef enum orient_type
{
    CENTER,
    UP,
    DOWN,
    RIGHT,
    LEFT,
    UNDEFINED
} orient;


/* cursmain.c */

extern struct window_procs curses_procs;

extern void FDECL(curses_exit_nhwindows, (const char *str));

extern void FDECL(curses_start_menu, (winid wid));

extern void FDECL(curses_add_menu, (winid wid, int glyph, const ANY_P * identifier,
		CHAR_P accelerator, CHAR_P group_accel, int attr, 
		const char *str, BOOLEAN_P presel));

extern void FDECL(curses_end_menu, (winid wid, const char *prompt));

extern int FDECL(curses_select_menu, (winid wid, int how, MENU_ITEM_P **selected));


/* curswins.c */

extern WINDOW *FDECL(curses_create_window, (int width, int height, orient orientation));

extern void FDECL(curses_destroy_win, (WINDOW *win));

extern WINDOW *FDECL(curses_get_nhwin, (winid wid));

extern void FDECL(curses_add_nhwin, (winid wid, int height, int width, int y,
 int x, orient orientation, BOOLEAN_P border));

extern void FDECL(curses_add_wid, (winid wid));

extern void FDECL(curses_refresh_nhwin, (winid wid));

extern void NDECL(curses_refresh_nethack_windows);

extern void FDECL(curses_del_nhwin, (winid wid));

extern void FDECL(curses_del_wid, (winid wid));

extern void FDECL(curses_putch, (winid wid, int x, int y, int ch, int color, int attrs));

extern void FDECL(curses_get_window_size, (winid wid, int *height, int *width));

extern boolean FDECL(curses_window_has_border, (winid wid));

extern boolean FDECL(curses_window_exists, (winid wid));

extern int FDECL(curses_get_window_orientation, (winid wid));

extern void FDECL(curses_get_window_xy, (winid wid, int *x, int *y));

extern void FDECL(curses_puts, (winid wid, int attr, const char *text));

extern void FDECL(curses_clear_nhwin, (winid wid));

extern void FDECL(curses_draw_map, (int sx, int sy, int ex, int ey));

extern boolean FDECL(curses_map_borders, (int *sx, int *sy, int *ex, int *ey,
 int ux, int uy));


/* cursmisc.c */

extern int NDECL(curses_read_char);

extern void FDECL(curses_toggle_color_attr, (WINDOW *win, int color, int attr, int onoff));

extern void FDECL(curses_bail, (const char *mesg));

extern winid FDECL(curses_get_wid, (int type));

extern char *FDECL(curses_copy_of, (const char *s));

extern int FDECL(curses_num_lines, (const char *str, int width));

extern char *FDECL(curses_break_str, (const char *str, int width, int line_num));

extern char *FDECL(curses_str_remainder, (const char *str, int width, int line_num));

extern boolean FDECL(curses_is_menu, (winid wid));

extern boolean FDECL(curses_is_text, (winid wid));

extern int FDECL(curses_convert_glyph, (int ch));

extern void FDECL(curses_move_cursor, (winid wid, int x, int y));

extern void NDECL(curses_prehousekeeping);

extern void NDECL(curses_posthousekeeping);

extern void FDECL(curses_view_file, (const char *filename, BOOLEAN_P must_exist));

extern void FDECL(curses_rtrim, (char *str));

extern int FDECL(curses_get_count, (int first_digit));

extern int FDECL(curses_convert_attr, (int attr));

extern int FDECL(curses_read_attrs, (char *attrs));

extern int FDECL(curses_convert_keys, (int key));

extern int FDECL(curses_get_mouse, (int *mousex, int *mousey, int *mod));

/* cursdial.c */

extern void FDECL(curses_line_input_dialog, (const char *prompt, char *answer, int buffer));

extern int FDECL(curses_character_input_dialog, (const char *prompt, const char *choices, CHAR_P def));

extern int FDECL(NDECL, (curses_ext_cmd));

extern void FDECL(curses_create_nhmenu, (winid wid));

extern void FDECL(curses_add_nhmenu_item, (winid wid, const ANY_P *identifier,
 CHAR_P accelerator, CHAR_P group_accel, int attr, const char *str,
 BOOLEAN_P presel));

extern void FDECL(curses_finalize_nhmenu, (winid wid, const char *prompt));

extern int FDECL(curses_display_nhmenu, (winid wid, int how, MENU_ITEM_P **_selected));

extern boolean FDECL(curses_menu_exists, (winid wid));

extern void FDECL(curses_del_menu, (winid wid));


/* cursstat.c */

extern void FDECL(curses_update_stats, (BOOLEAN_P redraw));

extern void NDECL(curses_decrement_highlight);


/* cursinit.c */

extern void NDECL(curses_create_main_windows);

extern void NDECL(curses_init_nhcolors);

extern void NDECL(curses_choose_character);

extern void NDECL(curses_init_options);

extern void NDECL(curses_display_splash_window);


/* cursmesg.c */

extern void FDECL(curses_message_win_puts, (const char *message, BOOLEAN_P recursed));

extern int NDECL(curses_more);

extern void NDECL(curses_clear_unhighlight_message_window);

extern void NDECL(curses_last_messages);

extern void NDECL(curses_init_mesg_history);

extern void NDECL(curses_prev_mesg);

extern void FDECL(curses_count_window, (const char *count_text));

#endif  /* WINCURS_H */

