#ifndef CURSMESG_H
#define CURSMESG_H


/* Global declarations */

void FDECL(curses_message_win_puts, (const char *message, BOOLEAN_P recursed));

int NDECL(curses_more);

void NDECL(curses_clear_unhighlight_message_window);

void NDECL(curses_last_messages);

void NDECL(curses_init_mesg_history);

void NDECL(curses_prev_mesg);

void FDECL(curses_count_window, (const char *count_text));

#endif  /* CURSMESG_H */
