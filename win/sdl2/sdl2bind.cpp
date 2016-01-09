// sdl2bind.cpp

extern "C" {
#include "hack.h"
}
#include "sdl2.h"
#include "sdl2interface.h"

//////////////////////////////////////////////////////////////////////////////
//                           Binding to the core                            //
//////////////////////////////////////////////////////////////////////////////
static NH_SDL2::SDL2Interface *interface;

void
sdl2_win_init(void)
{
    if (interface == NULL) {
        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER);
        IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF | IMG_INIT_WEBP);
        interface = new NH_SDL2::SDL2Interface;
        iflags.menu_tab_sep = TRUE;
        iflags.use_background_glyph = TRUE;
    }
}

static void
sdl2_init_nhwindows(int *argc, char **argv)
{
    sdl2_win_init();
    interface->sdl2_init_nhwindows(argc, argv);
}

static void
sdl2_player_selection(void)
{
    interface->sdl2_player_selection();
}

static void
sdl2_askname(void)
{
    interface->sdl2_askname();
}

static void
sdl2_get_nh_event(void)
{
    interface->sdl2_get_nh_event();
}

static void
sdl2_exit_nhwindows(const char *str)
{
    interface->sdl2_exit_nhwindows(str);
    delete interface;
    interface = NULL;
}

static void
sdl2_suspend_nhwindows(const char *str)
{
    interface->sdl2_suspend_nhwindows(str);
}

static void
sdl2_resume_nhwindows(void)
{
    interface->sdl2_resume_nhwindows();
}

static winid
sdl2_create_nhwindow(int type)
{
    return interface->sdl2_create_nhwindow(type);
}

static void
sdl2_clear_nhwindow(winid window)
{
    return interface->sdl2_clear_nhwindow(window);
}

static void
sdl2_display_nhwindow(winid window, BOOLEAN_P blocking)
{
    interface->sdl2_display_nhwindow(window, blocking);
}

static void
sdl2_destroy_nhwindow(winid window)
{
    return interface->sdl2_destroy_nhwindow(window);
}

static void
sdl2_curs(winid window, int x, int y)
{
    interface->sdl2_curs(window, x, y);
}

static void
sdl2_putstr(winid window, int attr, const char *str)
{
    interface->sdl2_putstr(window, attr, str);
}

static void
sdl2_putmixed(winid window, int attr, const char *str)
{
    interface->sdl2_putmixed(window, attr, str);
}

static void
sdl2_display_file(const char *filename, BOOLEAN_P complain)
{
    interface->sdl2_display_file(filename, complain);
}

static void
sdl2_start_menu(winid window)
{
    interface->sdl2_start_menu(window);
}

static void
sdl2_add_menu(winid window, int glyph, const anything* identifier, CHAR_P ch,
        CHAR_P gch, int attr, const char *str, BOOLEAN_P preselected)
{
    interface->sdl2_add_menu(window, glyph, identifier, ch, gch, attr, str,
            preselected);
}

static void
sdl2_end_menu(winid window, const char *prompt)
{
    interface->sdl2_end_menu(window, prompt ? prompt : "");
}

static int
sdl2_select_menu(winid window, int how, menu_item ** menu_list)
{
    return interface->sdl2_select_menu(window, how, menu_list);
}

static char
sdl2_message_menu(CHAR_P let, int how, const char *mesg)
{
    return interface->sdl2_message_menu(let, how, mesg);
}

static void
sdl2_update_inventory(void)
{
    interface->sdl2_update_inventory();
}

static void
sdl2_mark_synch(void)
{
    interface->sdl2_mark_synch();
}

static void
sdl2_wait_synch(void)
{
    if (interface != NULL) {
        interface->sdl2_wait_synch();
    } else {
        char s[80];
        printf("Press Enter to continue.\n");
        fgets(s, sizeof(s), stdin);
    }
}

#ifdef CLIPPING
static void
sdl2_cliparound(int x, int y)
{
    interface->sdl2_cliparound(x, y);
}
#endif

#ifdef POSITIONBAR
static void
sdl2_update_positionbar(char *features)
{
    interface->sdl2_update_positionbar(features);
}
#endif

static void
sdl2_print_glyph(winid window, XCHAR_P x, XCHAR_P y, int glyph, int bkglyph)
{
    interface->sdl2_print_glyph(window, x, y, glyph, bkglyph);
}

static void
sdl2_raw_print(const char *str)
{
    if (interface != NULL) {
        interface->sdl2_raw_print(str);
    } else {
        printf("%s\n", str);
    }
}

static void
sdl2_raw_print_bold(const char *str)
{
    interface->sdl2_raw_print_bold(str);
}

static int
sdl2_nhgetch(void)
{
    return interface->sdl2_nhgetch();
}

static int
sdl2_nh_poskey(int *x, int *y, int *mod)
{
    return interface->sdl2_nh_poskey(x, y, mod);
}

static void
sdl2_nhbell(void)
{
    interface->sdl2_nhbell();
}

static int
sdl2_doprev_message(void)
{
    return interface->sdl2_doprev_message();
}

static char
sdl2_yn_function(const char *ques, const char *choices, CHAR_P def)
{
    return interface->sdl2_yn_function(ques, choices, def);
}

static void
sdl2_getlin(const char *ques, char *input)
{
    interface->sdl2_getlin(ques, input);
}

static int
sdl2_get_ext_cmd(void)
{
    return interface->sdl2_get_ext_cmd();
}

static void
sdl2_number_pad(int state)
{
    interface->sdl2_number_pad(state);
}

static void
sdl2_delay_output(void)
{
    interface->sdl2_delay_output();
}

static void
sdl2_start_screen(void)
{
    interface->sdl2_start_screen();
}

static void
sdl2_end_screen(void)
{
    interface->sdl2_end_screen();
}

static void
sdl2_outrip(winid window, int how, time_t when)
{
    interface->sdl2_outrip(window, how, when);
}

static void
sdl2_preference_update(const char *pref)
{
    interface->sdl2_preference_update(pref);
}

#ifdef STATUS_VIA_WINDOWPORT
static void
sdl2_status_init(void)
{
    interface->sdl2_status_init();
}

static void
sdl2_status_finish(void)
{
    interface->sdl2_status_finish();
}

static void
sdl2_status_enablefield(int fieldidx, const char *nm, const char *fmt,
        BOOLEAN_P enable)
{
    interface->sdl2_status_enablefield(fieldidx, nm, fmt, enable);
}

static void
sdl2_status_update(int fldidx, genericptr_t ptr, int chg, int percent)
{
    interface->sdl2_status_update(fldidx, ptr, chg, percent);
}

#ifdef STATUS_HILITES
static void
sdl2_status_threshold(int fldidx, int thresholdtype, anything threshold,
        int behavior, int under, int over)
{
    interface->sdl2_status_threshold(fldidx, thresholdtype, threshold,
            behavior, under, over);
}
#endif
#endif

static void
sdl2_shieldeff(int x, int y)
{
    interface->sdl2_shieldeff(x, y);
}

struct window_procs sdl2_procs = {
    "sdl2",
    WC_COLOR |
    WC_HILITE_PET |
    WC_TILED_MAP |
    WC_PRELOAD_TILES |
    WC_TILE_WIDTH |
    WC_TILE_HEIGHT |
    WC_TILE_FILE |
    WC_MAP_MODE |
    WC_TILED_MAP |
    WC_MOUSE_SUPPORT |
    WC_FONT_MAP |
    WC_FONT_MENU |
    WC_FONT_MESSAGE |
    WC_FONT_STATUS |
    WC_FONT_TEXT |
    WC_FONTSIZ_MAP |
    WC_FONTSIZ_MENU |
    WC_FONTSIZ_MESSAGE |
    WC_FONTSIZ_STATUS |
    WC_FONTSIZ_TEXT |
    WC_PLAYER_SELECTION,
    WC2_FULLSCREEN,
    sdl2_init_nhwindows,
    sdl2_player_selection,
    sdl2_askname,
    sdl2_get_nh_event,
    sdl2_exit_nhwindows,
    sdl2_suspend_nhwindows,
    sdl2_resume_nhwindows,
    sdl2_create_nhwindow,
    sdl2_clear_nhwindow,
    sdl2_display_nhwindow,
    sdl2_destroy_nhwindow,
    sdl2_curs,
    sdl2_putstr,
    sdl2_putmixed,
    sdl2_display_file,
    sdl2_start_menu,
    sdl2_add_menu,
    sdl2_end_menu,
    sdl2_select_menu,
    sdl2_message_menu,
    sdl2_update_inventory,
    sdl2_mark_synch,
    sdl2_wait_synch,
#ifdef CLIPPING
    sdl2_cliparound,
#endif
#ifdef POSITIONBAR
    sdl2_update_positionbar,
#endif
    sdl2_print_glyph,
    sdl2_raw_print,
    sdl2_raw_print_bold,
    sdl2_nhgetch,
    sdl2_nh_poskey,
    sdl2_nhbell,
    sdl2_doprev_message,
    sdl2_yn_function,
    sdl2_getlin,
    sdl2_get_ext_cmd,
    sdl2_number_pad,
    sdl2_delay_output,
    sdl2_start_screen,
    sdl2_end_screen,
    sdl2_outrip,
    sdl2_preference_update,
    genl_getmsghistory,
    genl_putmsghistory,
#ifdef STATUS_VIA_WINDOWPORT
    sdl2_status_init,
    sdl2_status_finish,
    sdl2_status_enablefield,
    sdl2_status_update,
#ifdef STATUS_HILITES
    sdl2_status_threshold,
#endif
#endif
    genl_can_suspend_no,

    /* Optional functions begin here */
    sdl2_shieldeff,
};
