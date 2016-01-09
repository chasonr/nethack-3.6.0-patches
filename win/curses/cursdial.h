#ifndef CURSDIAL_H
#define CURSDIAL_H

#ifdef MENU_COLOR
# ifdef MENU_COLOR_REGEX
#  include <regex.h>
# endif
#endif


/* Global declarations */

void FDECL(curses_line_input_dialog, (const char *prompt, char *answer, int buffer));

int FDECL(curses_character_input_dialog, (const char *prompt, const char *choices, CHAR_P def));

int FDECL(curses_ext_cmd, (void));

void FDECL(curses_create_nhmenu, (winid wid));

void FDECL(curses_add_nhmenu_item, (winid wid, const ANY_P *identifier,
 CHAR_P accelerator, CHAR_P group_accel, int attr, const char *str,
 BOOLEAN_P presel));

void FDECL(curses_finalize_nhmenu, (winid wid, const char *prompt));

int FDECL(curses_display_nhmenu, (winid wid, int how, MENU_ITEM_P **_selected));

boolean FDECL(curses_menu_exists, (winid wid));

void FDECL(curses_del_menu, (winid wid));



#endif  /* CURSDIAL_H */
