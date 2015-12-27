// sdl2interface.h

#ifndef SDL2INTERFACE_H
#define SDL2INTERFACE_H

#include <list>
#include <map>
#include <string>
#include <vector>
#include "sdl2.h"

class SDL2Font;

namespace NH_SDL2
{

class SDL2Window;
class SDL2MapWindow;
class SDL2MessageWindow;
class SDL2StatusWindow;
#ifdef POSITIONBAR
class SDL2PositionBar;
#endif

class SDL2Interface
{
public:
    SDL2Interface(void);
    ~SDL2Interface(void);

    // External interface
    void  sdl2_init_nhwindows(int *argc, char **argv);
    void  sdl2_player_selection(void);
    void  sdl2_askname(void);
    void  sdl2_get_nh_event(void);
    void  sdl2_exit_nhwindows(const char *str);
    void  sdl2_suspend_nhwindows(const char *str);
    void  sdl2_resume_nhwindows(void);
    winid sdl2_create_nhwindow(int type);
    void  sdl2_clear_nhwindow(winid window);
    void  sdl2_display_nhwindow(winid window, bool blocking);
    void  sdl2_destroy_nhwindow(winid window);
    void  sdl2_curs(winid window, int x, int y);
    void  sdl2_putstr(winid window, int attr, const std::string& str);
    void  sdl2_putmixed(winid window, int attr, const std::string& str);
    void  sdl2_display_file(const char *filename, bool complain);
    void  sdl2_start_menu(winid window);
    void  sdl2_add_menu(winid window, int glyph, const anything* identifier,
                    char ch, char gch, int attr, const std::string& str,
                    bool preselected);
    void  sdl2_end_menu(winid window, const std::string& prompt);
    int   sdl2_select_menu(winid window, int how, menu_item ** menu_list);
    char  sdl2_message_menu(char let, int how, const char *mesg);
    void  sdl2_update_inventory(void);
    void  sdl2_mark_synch(void);
    void  sdl2_wait_synch(void);
#ifdef CLIPPING
    void  sdl2_cliparound(int x, int y);
#endif
#ifdef POSITIONBAR
    void  sdl2_update_positionbar(char *features);
#endif
    void  sdl2_print_glyph(winid window, xchar x, xchar y, int glyph, int bkglyph);
    void  sdl2_raw_print(const char *str);
    void  sdl2_raw_print_bold(const char *str);
    int   sdl2_nhgetch(void);
    int   sdl2_nh_poskey(int *x, int *y, int *mod);
    void  sdl2_nhbell(void);
    int   sdl2_doprev_message(void);
    char  sdl2_yn_function(const char *ques, const char *choices, char def);
    void  sdl2_getlin(const char *ques, char *input);
    int   sdl2_get_ext_cmd(void);
    void  sdl2_number_pad(int state);
    void  sdl2_delay_output(void);
    void  sdl2_start_screen(void);
    void  sdl2_end_screen(void);
    void  sdl2_outrip(winid window, int how, time_t when);
    void  sdl2_preference_update(const char *pref);
#ifdef STATUS_VIA_WINDOWPORT
    void  sdl2_status_init(void);
    void  sdl2_status_finish(void);
    void  sdl2_status_enablefield(int, const char *, const char *, bool);
    void  sdl2_status_update(int, genericptr_t, int, int);
#ifdef STATUS_HILITES
    void  sdl2_status_threshold(int, int, anything, int, int, int);
#endif
#endif

    // For use by windows
    int width(void);
    int height(void);
    void createNotify(SDL2Window *window);
    void closeNotify(SDL2Window *window);
    void resizeNotify(SDL2Window *window);
    void redraw(void);
    void blit(SDL2Window *window, const SDL_Rect& window_rect,
            SDL_Surface *surface, const SDL_Rect& surface_rect);
    void blit(SDL2Window *window,
            SDL_Surface *surface, const SDL_Rect& surface_rect);
    void blit(SDL2Window *window, const SDL_Rect& window_rect,
            SDL_Surface *surface);
    void blit(SDL2Window *window, SDL_Surface *surface);
    void fill(SDL2Window *window, const SDL_Rect& window_rect, SDL_Color color);
    void fill(SDL2Window *window, SDL_Color color);

    utf32_t getKey(bool cmd = false, int *x = NULL, int *y = NULL, int *mod = NULL);

    SDL_PixelFormat *pixelFormat(void);

    void setMessage(const std::string& message);
    void fadeMessage(void);

private:
    // Given a winid, find the window
    winid next_winid;
    std::map<winid, SDL2Window *> window_map;

    SDL_Window *main_window;

    // Last window in this vector appears on top
    std::list<SDL2Window *> window_stack;

    // Windows in the main display
    // These pointers are not custodial; the windows are also in window_stack
    // and will be deleted that way
    SDL2MessageWindow *message_window;
    SDL2MapWindow *map_window;
#ifdef POSITIONBAR
    SDL2PositionBar *posbar_window;
#endif
    SDL2StatusWindow *status_window;

    // To arrange the message, map and status windows
    void arrangeWindows(void);

    // Queue of characters previously typed
    std::basic_string<utf32_t> key_queue;
    std::size_t key_queue_index;

    // For window manager events and other events besides keys
    void nonKeyEvent(const SDL_Event& event);

    // On-screen message
    std::string m_message;
    Uint32 m_message_time;
    SDL_TimerID m_timer_id;
    SDL2Font *m_font;
    void doMessageFade(void);
    static Uint32 timerCallback(Uint32 interval, void *param);

    // Video mode for full screen
    int m_video_mode;
    int m_max_bits;
    void nextVideoMode(void);
    void newDisplay(void);
};

nhsym chrConvert(nhsym ch);

}

#endif
