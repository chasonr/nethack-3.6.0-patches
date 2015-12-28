// sdl2interface.cpp

extern "C" {
#include "hack.h"
#include "dlb.h"
}
#include "func_tab.h"
#include <algorithm>
#include <stdexcept>
#include "sdl2.h"
#include "sdl2interface.h"
#include "sdl2window.h"
#include "sdl2message.h"
#include "sdl2map.h"
#include "sdl2posbar.h"
#include "sdl2status.h"
#include "sdl2menu.h"
#include "sdl2text.h"
#include "sdl2plsel.h"
#include "sdl2getlin.h"
#include "sdl2extcmd.h"
#include "sdl2font.h"


namespace NH_SDL2
{

//////////////////////////////////////////////////////////////////////////////
//                         Main application window                          //
//////////////////////////////////////////////////////////////////////////////

SDL2Interface::SDL2Interface(void) :
    next_winid(0),
    main_window(NULL),
    window_top(0),
    message_window(NULL),
    map_window(NULL),
#ifdef POSITIONBAR
    posbar_window(NULL),
#endif
    status_window(NULL),
    key_queue_head(0),
    key_queue_tail(0),
    m_message(NULL),
    m_message_time(0),
    m_timer_id(0),
    m_font(NULL),
    m_video_mode(0),
    m_max_bits(0)
{
    main_window = SDL_CreateWindow("NetHack",
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            1000, 600, SDL_WINDOW_RESIZABLE);
    if (iflags.wc2_fullscreen) {
        SDL_SetWindowFullscreen(main_window, SDL_WINDOW_FULLSCREEN);
        newDisplay();
    }

    // Message font
    const char *font = iflags.wc_font_message;
    if (font == NULL || font[0] == '\0') {
        font = SDL2Font::defaultSerifFont();
    }
    int size = iflags.wc_fontsiz_message;
    if (size == 0) {
        size = 20;
    }
    m_font = new SDL2Font(font, size);

    m_timer_id = SDL_AddTimer(100, SDL2Interface::timerCallback, this);
}

SDL2Interface::~SDL2Interface(void)
{
    SDL_RemoveTimer(m_timer_id);
    delete m_font;
    delete[] m_message;
    SDL_DestroyWindow(main_window);
    // The SDL2Window destructor calls closeNotify, which removes the window
    // from window_stack
    while (window_top != 0) {
        delete window_stack[window_top - 1].window;
    }
}

int SDL2Interface::width(void)
{
    int w, h;

    SDL_GetWindowSize(main_window, &w, &h);
    return w;
}

int SDL2Interface::height(void)
{
    int w, h;

    SDL_GetWindowSize(main_window, &w, &h);
    return h;
}

void SDL2Interface::createNotify(SDL2Window *window)
{
    if (window_top >= SIZE(window_stack)) {
        panic("Too many windows created\n");
    }
    window_stack[window_top].window = window;
    window_stack[window_top].id = -1;
    ++window_top;
}

void SDL2Interface::closeNotify(SDL2Window *window)
{
    size_t i;

    for (i = 0; i < window_top && window_stack[i].window != window; ++i) {}
    for (; i + 1 < window_top; ++i) {
        window_stack[i] = window_stack[i + 1];
    }
    --window_top;

    if (window == message_window) { message_window = NULL; }
    if (window == map_window    ) { map_window     = NULL; }
#ifdef POSITIONBAR
    if (window == posbar_window ) { posbar_window  = NULL; }
#endif
    if (window == status_window ) { status_window  = NULL; }
}

void SDL2Interface::resizeNotify(SDL2Window * /*window*/)
{
}

void SDL2Interface::nextVideoMode(void)
{
    int display = SDL_GetWindowDisplayIndex(main_window);
    int num_modes = SDL_GetNumDisplayModes(display);
    SDL_DisplayMode mode;

    // Find next mode with the maximum number of bits per pixel
    for (++m_video_mode; m_video_mode < num_modes; ++m_video_mode) {
        SDL_GetDisplayMode(display, m_video_mode, &mode);
        unsigned bits = SDL_BITSPERPIXEL(mode.format);
        if (bits >= m_max_bits) { break; }
    }

    // If we've passed the last mode for this display, try the next one
    if (m_video_mode >= num_modes) {
        newDisplay();
    } else {
        SDL_GetDisplayMode(display, m_video_mode, &mode);
        SDL_SetWindowFullscreen(main_window, 0);
        SDL_SetWindowDisplayMode(main_window, &mode);
        SDL_SetWindowFullscreen(main_window, SDL_WINDOW_FULLSCREEN);
    }
}

void SDL2Interface::newDisplay(void)
{
    int display = SDL_GetWindowDisplayIndex(main_window);
    int num_modes = SDL_GetNumDisplayModes(display);
    SDL_DisplayMode mode;

    // First pass; find the maximum number of bits per pixel
    m_max_bits = 0;
    for (int j = 0; j < num_modes; ++j)
    {
        SDL_GetDisplayMode(display, j, &mode);
        unsigned bits = SDL_BITSPERPIXEL(mode.format);
        printf("%s:  mode %d: width=%d height=%d depth=%d\n",
                __func__, j, mode.w, mode.h, bits);
        if (bits < m_max_bits) m_max_bits = bits;
    }

    // Second pass; find the first (that is, highest resolution) mode with the
    // maximum number of bits per pixel
    m_video_mode = 0;
    for (int j = 0; j < num_modes; ++j)
    {
        SDL_GetDisplayMode(display, j, &mode);
        unsigned bits = SDL_BITSPERPIXEL(mode.format);
        if (bits >= m_max_bits) {
            printf("%s:  selected mode: width=%d height=%d depth=%d\n",
                    __func__, mode.w, mode.h, bits);
            m_video_mode = j;
            break;
        }
    }

    SDL_GetDisplayMode(display, m_video_mode, &mode);
    SDL_SetWindowFullscreen(main_window, 0);
    SDL_SetWindowDisplayMode(main_window, &mode);
    SDL_SetWindowFullscreen(main_window, SDL_WINDOW_FULLSCREEN);
}

void SDL2Interface::redraw(void)
{
    SDL_Surface *main_surface = SDL_GetWindowSurface(main_window);
    Uint32 background = SDL_MapRGBA(main_surface->format, 0, 0, 0, 255);
    SDL_SetClipRect(main_surface, NULL);
    SDL_FillRect(main_surface, NULL, background);

    // Draw each window; the blit method will perform any clipping
    for (std::size_t i = 0; i < window_top; ++i) {
        if (window_stack[i].window->isVisible()) {
            window_stack[i].window->redraw();
        }
    }

    // If a message is present, alpha blend it on top of the screen
    if (m_message != NULL) {
        SDL_SetClipRect(main_surface, NULL);
        Uint32 ticks = SDL_GetTicks();
        unsigned alpha;
        if (m_message_time == 0) {
            alpha = 255;
        } else if (ticks - m_message_time >= 2000) {
            alpha = 0;
        } else {
            alpha = (2000 - (ticks - m_message_time)) * 255 / 2000;
        }
        SDL_Color yellow = { 255, 255,  0, static_cast<Uint8>(alpha) };
        SDL_Color black  = {   0,   0,  0, static_cast<Uint8>(alpha) };
        SDL_Surface *text = m_font->render(m_message, yellow, black);
        SDL_Rect rect = { main_surface->w - text->w, 0, text->w, text->h };
        SDL_BlitSurface(text, NULL, main_surface, &rect);
        SDL_FreeSurface(text);
    }

    SDL_UpdateWindowSurface(main_window);
}

void SDL2Interface::blit(SDL2Window *window, const SDL_Rect& window_rect,
        SDL_Surface *surface, const SDL_Rect& surface_rect)
{
    SDL_Surface *main_surface = SDL_GetWindowSurface(main_window);
    SDL_Rect window_rect2;

    if (!window->isVisible()) { return; }

    // Shift the window rectangle to main surface coordinates
    window_rect2 = window_rect;
    window_rect2.x += window->xmin();
    window_rect2.y += window->ymin();

    // Set the clipping rectangle
    SDL_SetClipRect(main_surface, &window_rect2);

    // Draw
    SDL_BlitSurface(surface, &surface_rect, main_surface, &window_rect2);
}

void SDL2Interface::blit(SDL2Window *window,
        SDL_Surface *surface, const SDL_Rect& /*surface_rect*/)
{
    SDL_Rect rect = { 0, 0, window->width(), window->height() };

    blit(window, rect, surface);
}

void SDL2Interface::blit(SDL2Window *window, const SDL_Rect& window_rect,
        SDL_Surface *surface)
{
    SDL_Rect rect = { 0, 0, surface->w, surface->h };

    blit(window, window_rect, surface, rect);
}

void SDL2Interface::blit(SDL2Window *window, SDL_Surface *surface)
{
    SDL_Rect rect = { 0, 0, surface->w, surface->h };

    blit(window, surface, rect);
}

void SDL2Interface::fill(SDL2Window *window, const SDL_Rect& window_rect, SDL_Color color)
{
    if (!window->isVisible()) { return; }

    SDL_Surface *main_surface = SDL_GetWindowSurface(main_window);
    SDL_Rect window_rect2;

    // Shift the window rectangle to main surface coordinates
    window_rect2 = window_rect;
    window_rect2.x += window->xmin();
    window_rect2.y += window->ymin();

    // Set the clipping rectangle
    SDL_SetClipRect(main_surface, &window_rect2);

    // Draw
    Uint32 cindex = SDL_MapRGBA(main_surface->format,
            color.r, color.g, color.b, color.a);
    if (color.a == 255 || SDL_BYTESPERPIXEL(main_surface->format->format) != 4) {
        SDL_FillRect(main_surface, &window_rect2, cindex);
    } else {
        if (SDL_MUSTLOCK(main_surface)) {
            SDL_LockSurface(main_surface);
        }
        uint32_t *row = (uint32_t *)((char *)main_surface->pixels + window_rect2.y*main_surface->pitch);
        uint8_t b1a = (uint8_t)(cindex >> 24);
        uint8_t b2a = (uint8_t)(cindex >> 16);
        uint8_t b3a = (uint8_t)(cindex >>  8);
        uint8_t b4a = (uint8_t)(cindex >>  0);
        for (int y = 0; y < window_rect2.h; ++y) {
            if (window_rect2.y + y < 0) {
                row = (uint32_t *)((char *)row + main_surface->pitch);
                continue;
            }
            if (window_rect2.y + y >= main_surface->h) { break; }
            uint32_t *pixel = row + window_rect2.x;
            for (int x = 0; x < window_rect2.w; ++x) {
                if (window_rect2.x + x < 0) {
                    ++pixel;
                    continue;
                }
                if (window_rect2.x + x >= main_surface->w) { break; }

                // We can alpha blend even if the pixel format has no alpha
                uint8_t b1b = (uint8_t)(*pixel >> 24);
                uint8_t b2b = (uint8_t)(*pixel >> 16);
                uint8_t b3b = (uint8_t)(*pixel >>  8);
                uint8_t b4b = (uint8_t)(*pixel >>  0);
                switch (SDL_PIXELORDER(main_surface->format->format)) {
                case SDL_PACKEDORDER_ARGB:
                case SDL_PACKEDORDER_ABGR:
                case SDL_PACKEDORDER_XRGB:
                case SDL_PACKEDORDER_XBGR:
                    b2b = ((b2a * color.a) + (b2b * (255 - color.a))) / 255;
                    b3b = ((b3a * color.a) + (b3b * (255 - color.a))) / 255;
                    b4b = ((b4a * color.a) + (b4b * (255 - color.a))) / 255;
                    b1b = 255;
                    break;

                case SDL_PACKEDORDER_RGBA:
                case SDL_PACKEDORDER_BGRA:
                case SDL_PACKEDORDER_RGBX:
                case SDL_PACKEDORDER_BGRX:
                    b1b = ((b1a * color.a) + (b1b * (255 - color.a))) / 255;
                    b2b = ((b2a * color.a) + (b2b * (255 - color.a))) / 255;
                    b3b = ((b3a * color.a) + (b3b * (255 - color.a))) / 255;
                    b4b = 255;
                    break;

                default:
                    // We shouldn't get here; alpha blending is unsupported
                    // on pixel formats less than 32 bits
                    b1b = b1a;
                    b2b = b2a;
                    b3b = b3a;
                    b4b = b4a;
                    break;
                }
                *pixel = ((uint32_t)b1b << 24)
                       | ((uint32_t)b2b << 16)
                       | ((uint32_t)b3b <<  8)
                       | ((uint32_t)b4b <<  0);

                ++pixel;
            }
            row = (uint32_t *)((char *)row + main_surface->pitch);
        }
        if (SDL_MUSTLOCK(main_surface)) {
            SDL_UnlockSurface(main_surface);
        }
    }
}

void SDL2Interface::fill(SDL2Window *window, SDL_Color color)
{
    SDL_Rect rect = { 0, 0, window->width() - 1, window->height() - 1 };

    fill(window, rect, color);
}

void SDL2Interface::arrangeWindows(void)
{
    if (message_window == NULL) { return; }
    if (map_window == NULL)     { return; }
#ifdef POSITIONBAR
    if (posbar_window == NULL)  { return; }
#endif
    if (status_window == NULL)  { return; }

    int message_hint = message_window->heightHint();
    int map_hint = map_window->heightHint();
#ifdef POSITIONBAR
    int posbar_hint = posbar_window->isVisible() ? posbar_window->heightHint() : 0;
#endif
    int status_hint = status_window->heightHint();

    // The status window hint is the height of its two lines
    int width, height;
    SDL_GetWindowSize(main_window, &width, &height);
    status_window->resize(0, height - status_hint, width - 1, height - 1);
    height -= status_hint;

    // The position bar window hint is the height of its one line
#ifdef POSITIONBAR
    posbar_window->resize(0, height - posbar_hint, width - 1, height - 1);
    height -= posbar_hint;
#endif

    if (height >= message_hint + map_hint) {
        // Enough height for all.  Give the map window its full height and
        // give the excess to the message window
        map_window->resize(0, height - map_hint, width - 1, height - 1);
        height -= map_hint;
        message_window->resize(0, 0, width - 1, height - 1);
    } else {
        // Not enough height for all.  Make the message window one line high
        // and give the rest to the map window
        message_window->resize(0, 0, width - 1, message_hint - 1);
        if (height < 1) { height = 1; }
        map_window->resize(0, message_hint, width - 1, height);
    }

    map_window->clipAround();
}

void SDL2Interface::sdl2_init_nhwindows(int * /*argc*/, char ** /*argv*/)
{
    // Place holder
}

void SDL2Interface::sdl2_player_selection(void)
{
    if (!SDL2PlayerSelect(this)) {
        clearlocks();
        sdl2_exit_nhwindows("Quit from character selection.");
        terminate(EXIT_SUCCESS);
    }
}

void SDL2Interface::sdl2_askname(void)
{
    str_context ctx = str_open_context("SDL2Interface::sdl2_askname");
    SDL2GetLine window(this);
    char *str;

    str = window.getLine("What is your name?");
    if (str != NULL) {
        uni_copy8(plname, SIZE(plname), str);
    } else {
        plname[0] = '\0';
    }
    str_close_context(ctx);
}

void SDL2Interface::sdl2_get_nh_event(void)
{
    // Place holder
}

void SDL2Interface::sdl2_exit_nhwindows(const char *str)
{
    printf("%s\n", str);
}

void SDL2Interface::sdl2_suspend_nhwindows(const char * /*str*/)
{
    // Place holder
}

void SDL2Interface::sdl2_resume_nhwindows(void)
{
    // Place holder
}

winid SDL2Interface::sdl2_create_nhwindow(int type)
{
    SDL2Window *window = NULL;
    switch (type) {
    case NHW_MESSAGE:
        window = message_window = new SDL2MessageWindow(this);
        arrangeWindows();
        break;

    case NHW_STATUS:
        window = status_window = new SDL2StatusWindow(this);
        arrangeWindows();
        break;

    case NHW_MAP:
        window = map_window = new SDL2MapWindow(this);
#ifdef POSITIONBAR
        posbar_window = new SDL2PositionBar(this, map_window);
#endif
        arrangeWindows();
        break;

    case NHW_MENU:
        window = new SDL2Menu(this);
        break;

    case NHW_TEXT:
        window = new SDL2Text(this);
        break;
    }

    /* Window is already pushed on the stack */
    /* Return an unused window ID */
    while (findWindow(next_winid) != NULL) {
        if (next_winid == std::numeric_limits<winid>::max()) {
            next_winid = 0;
        } else {
            ++next_winid;
        }
    }
    for (std::size_t i = window_top; i-- != 0; ) {
        if (window_stack[i].window == window) {
            window_stack[i].id = next_winid;
            break;
        }
    }
    return next_winid;
}

SDL2Window *SDL2Interface::findWindow(winid id)
{
    for (size_t i = 0; i < window_top; ++i) {
        if (id == window_stack[i].id) {
            return window_stack[i].window;
        }
    }
    return NULL;
}

void SDL2Interface::sdl2_clear_nhwindow(winid window)
{
    SDL2Window *winp = findWindow(window);

    winp->clear();
}

void SDL2Interface::sdl2_display_nhwindow(winid window, bool blocking)
{
    SDL2Window *winp = findWindow(window);

    winp->setVisible(true);
    redraw();
    if (blocking && (winp == message_window || winp == map_window)) {
        if (winp == map_window) {
            flush_screen(TRUE);
        }
        message_window->more();
    }
}

void SDL2Interface::sdl2_destroy_nhwindow(winid window)
{
    SDL2Window *winp = findWindow(window);

    delete winp; // calls closeNotify, which removes the window from window_stack
}

void SDL2Interface::sdl2_curs(winid window, int x, int y)
{
    SDL2Window *winp = findWindow(window);

    winp->setCursor(x, y);
}

void SDL2Interface::sdl2_putstr(winid window, int attr, const char *str)
{
    SDL2Window *winp = findWindow(window);

    winp->putStr(attr, str);
}

void SDL2Interface::sdl2_putmixed(winid window, int attr, const char *str)
{
    SDL2Window *winp = findWindow(window);

    winp->putMixed(attr, str);
}

void SDL2Interface::sdl2_display_file(const char *filename, bool complain)
{
    dlb *file;

    file = dlb_fopen(filename, "r");
    if (file != NULL) {
        SDL2Text window(this);

        while (true) {
            char line[BUFSZ];
            std::size_t len;

            if (dlb_fgets(line, sizeof(line), file) == NULL) { break; }
            len = strlen(line);
            if (len != 0 && line[len-1] == '\n') {
                line[len-1] = '\0';
            }
            window.putStr(0, line);
        }
        dlb_fclose(file);

        window.setVisible(true);
    } else if (complain) {
        char msg[BUFSZ];

        snprintf(msg, SIZE(msg), "Could not open file %1$s", filename);
        message_window->putStr(0, msg);
        redraw();
    }
}

void SDL2Interface::sdl2_start_menu(winid window)
{
    SDL2Window *winp = findWindow(window);

    winp->startMenu();
}

void SDL2Interface::sdl2_add_menu(winid window, int glyph,
        const anything* identifier, char ch, char gch, int attr,
        const char *str, bool preselected)
{
    SDL2Window *winp = findWindow(window);

    winp->addMenu(glyph, identifier, ch, gch, attr, str, preselected);
}

void SDL2Interface::sdl2_end_menu(winid window, const char *prompt)
{
    SDL2Window *winp = findWindow(window);

    winp->endMenu(prompt);
}

int SDL2Interface::sdl2_select_menu(winid window, int how,
        menu_item **menu_list)
{
    SDL2Window *winp = findWindow(window);

    return winp->selectMenu(how, menu_list);
}

utf32_t SDL2Interface::getKey(bool cmd, int *x, int *y, int *mod)
{
    StringContext ctx("SDL2Interface::getKey");
    utf32_t ch;
    bool shift;

    if (mod) {
        // If we inadvertently return a null character, don't make it into
        // a mouse click
        *mod = 0;
    }

    // Return characters queued from a previous event
    if (key_queue_head != key_queue_tail) {
        return key_queue[key_queue_head++];
    }

    redraw();

    SDL_Event event;

    while (true) {
        SDL_WaitEvent(&event);
        nonKeyEvent(event);

        switch (event.type) {
        case SDL_KEYDOWN:
            ch = event.key.keysym.sym;
            shift = event.key.keysym.mod & (KMOD_LSHIFT | KMOD_RSHIFT);
#if 0 // debug
            printf("scancode=%X sym=%X mod=%X\n",
                    event.key.keysym.scancode,
                    event.key.keysym.sym,
                    event.key.keysym.mod);
#endif
            // Control codes and special keys don't produce a text input event
            switch (ch) {

            // Function keys
            case SDLK_F1:
                setMessage("F3-Window/FullScreen F4-VidMode F5/Tab - Tiles/ASCII F6-PositionBar F8-ZmMode");
                //setMessage("F3-FitMode F4-VidMode F5/TAB-Tiles/ASCII F6-PositionBar F8-ZmMode F9-ZmOut F10-ZmIn");
                redraw();
                break;

            case SDLK_F3:
                iflags.wc2_fullscreen = !iflags.wc2_fullscreen;
                if (iflags.wc2_fullscreen) {
                    SDL_SetWindowFullscreen(main_window, SDL_WINDOW_FULLSCREEN);
                    setMessage("Full screen mode");
                    m_video_mode = 0;
                } else {
                    SDL_SetWindowFullscreen(main_window, 0);
                    setMessage("Windowed mode");
                }
                arrangeWindows();
                redraw();
                break;

            case SDLK_F4:
                {
                    int width, height;
                    char msg[BUFSZ];

                    nextVideoMode();
                    arrangeWindows();
                    SDL_GetWindowSize(main_window, &width, &height);

                    snprintf(msg, SIZE(msg), "Video Mode: %dx%d", width, height);
                    setMessage(msg);
                    redraw();
                }
                break;

            case '\t': // Tab key, but not control-I
            case SDLK_F5:
                map_window->toggleTileMode();
                arrangeWindows();
                redraw();
                break;

#ifdef POSITIONBAR
            case SDLK_F6:
                posbar_window->setVisible(!posbar_window->isVisible());
                setMessage(posbar_window->isVisible()
                        ? "Position Bar: On"
                        : "Position Bar: Off");
                arrangeWindows();
                redraw();
                break;
#endif

            case SDLK_F8:
                map_window->nextZoomMode();
                arrangeWindows();
                redraw();
                break;

            // For the numeric keypad, the SDL_TEXTINPUT events seem to work
            // just fine when num_pad is set, at least on Mac.
            // TODO:  Test on Windows, Linux

            // Cursor keys

            case SDLK_DOWN:
                if (cmd) {
                    return iflags.num_pad ? '2' : shift ? 'J' : 'j';
                } else {
                    return ch;
                }

            case SDLK_LEFT:
                if (cmd) {
                    return iflags.num_pad ? '4' : shift ? 'H' : 'h';
                } else {
                    return ch;
                }

            case SDLK_RIGHT:
                if (cmd) {
                    return iflags.num_pad ? '6' : shift ? 'L' : 'l';
                } else {
                    return ch;
                }

            case SDLK_UP:
                if (cmd) {
                    return iflags.num_pad ? '8' : shift ? 'K' : 'k';
                } else {
                    return ch;
                }

            case SDLK_HOME:
                return MENU_FIRST_PAGE;

            case SDLK_PAGEUP:
                return MENU_PREVIOUS_PAGE;

            case SDLK_END:
                return MENU_LAST_PAGE;

            case SDLK_PAGEDOWN:
                return MENU_NEXT_PAGE;

            default:
                if (ch < 0x20 || (0x7F <= ch && ch <= 0x9F) || 0x10FFFF < ch) {
                    // Control characters such as Escape, Backspace, Delete, and
                    // keys such as arrows, function keys, etc. that do not strike
                    // a character
                    return ch;
                } else if ('A' <= ch && ch <= '~'
                && (event.key.keysym.mod & (KMOD_LCTRL | KMOD_RCTRL)) != 0) {
                    // CTRL+letter
                    return ch & 0x1F;
                }
            }
            break;

        case SDL_KEYUP:
            fadeMessage();
            break;

        case SDL_TEXTINPUT:
            {
                utf32_t *text = uni_8to32(event.text.text);
                if (text[0] != 0) {
                    ch = text[0];
                    key_queue_head = 0;
                    key_queue_tail = uni_length32(text + 1);
                    if (key_queue_tail > SIZE(key_queue)) {
                        key_queue_tail = SIZE(key_queue);
                    }
                    memcpy(key_queue, text + 1, key_queue_tail * sizeof(text[0]));
                    return ch;
                }
            }
            break;

        case SDL_MOUSEBUTTONDOWN:
            if (cmd && x != NULL && y != NULL
            && (event.button.button == SDL_BUTTON_LEFT || event.button.button == SDL_BUTTON_RIGHT)) {
                Sint32 xm = event.button.x;
                Sint32 ym = event.button.y;
                if (map_window->xmin() <= xm && xm <= map_window->xmax()
                &&  map_window->ymin() <= ym && ym <= map_window->ymax()
                &&  map_window->mapMouse(
                            xm - map_window->xmin(),
                            ym - map_window->ymin(),
                            x, y)) {
                    *mod = event.button.button == SDL_BUTTON_LEFT ? CLICK_1 : CLICK_2;
                    return 0;
                }
            }
            break;
        // TODO:  process SDL_QUIT, others
        }
    }
}

SDL_PixelFormat *SDL2Interface::pixelFormat(void)
{
    SDL_Surface *main_surface = SDL_GetWindowSurface(main_window);
    return main_surface->format;
}

void SDL2Interface::nonKeyEvent(const SDL_Event& event)
{
    switch (event.type) {
    case SDL_WINDOWEVENT:
#if 0
        printf("%s: event=%d data1=%d data2=%d\n",
                __func__,
                event.window.event,
                event.window.data1,
                event.window.data2);
#endif
        switch (event.window.event) {
        case SDL_WINDOWEVENT_RESIZED:
            arrangeWindows();
            redraw();
            break;
        }
        break;

    case SDL_USEREVENT:     // Timer tick
        {
            // Prevent timer events from piling up on the queue
            SDL_Event event2;

            while (SDL_PeepEvents(&event2, 1, SDL_GETEVENT,
                    SDL_USEREVENT, SDL_USEREVENT) > 0) {}

            // Fade the screen message out
            doMessageFade();
        }
        break;
    }
}

Uint32 SDL2Interface::timerCallback(Uint32 interval, void * /*param*/)
{
    // The timer callback is executed in a separate thread and we cannot do
    // much directly.  Instead, we must post an event to the event queue.
    //SDL2Interface *ours = reinterpret_cast<SDL2Interface *>(param);

    SDL_Event event;

    memset(&event, 0, sizeof(event));
    event.type = event.user.type = SDL_USEREVENT;
    event.user.code = 0;
    event.user.data1 = 0;
    event.user.data2 = 0;

    SDL_PushEvent(&event);
    return interval;
}

void SDL2Interface::doMessageFade(void)
{
    if (m_message_time != 0 && m_message != NULL) {
        // Message is present and it is being faded
        if (SDL_GetTicks() - m_message_time > 2000) {
            // Message has timed out
            delete[] m_message;
            m_message = NULL;
            m_message_time = 0;
        }

        redraw();
    }
}

void SDL2Interface::setMessage(const char *message)
{
    delete[] m_message;
    m_message = new char[strlen(message) + 1];
    strcpy(m_message, message);
    m_message_time = 0; // Not fading yet
}

void SDL2Interface::fadeMessage(void)
{
    if (m_message_time == 0 && m_message != NULL) {
        m_message_time = SDL_GetTicks();
        if (m_message_time == 0) {
            m_message_time = 1;
        }
    }
}

char SDL2Interface::sdl2_message_menu(char let, int how, const char *mesg)
{
    // Place holder
    return genl_message_menu(let, how, mesg);
}

void SDL2Interface::sdl2_update_inventory(void)
{
    // Place holder
}

void SDL2Interface::sdl2_mark_synch(void)
{
    redraw();
}

void SDL2Interface::sdl2_wait_synch(void)
{
    redraw();
}

#ifdef CLIPPING
void SDL2Interface::sdl2_cliparound(int x, int y)
{
    map_window->clipAround(x, y);
}
#endif

#ifdef POSITIONBAR
void SDL2Interface::sdl2_update_positionbar(char *features)
{
    posbar_window->updatePositionBar(features);
}
#endif

void SDL2Interface::sdl2_print_glyph(winid window, xchar x, xchar y,
        int glyph, int /*bkglyph*/)
{
    SDL2Window *winp = findWindow(window);

    winp->printGlyph(x, y, glyph);
}

void SDL2Interface::sdl2_raw_print(const char *str)
{
    if (message_window != NULL) {
        message_window->putStr(0, str);
    } else {
        printf("%s\n", str);
    }
}

void SDL2Interface::sdl2_raw_print_bold(const char *str)
{
    if (message_window != NULL) {
        message_window->putStr(ATR_BOLD, str);
    } else {
        printf("%s\n", str);
    }
}

int SDL2Interface::sdl2_nhgetch(void)
{
    utf32_t ch;

    // For now, accept only ASCII
    do {
        ch = getKey(false);
    } while (ch <= 0 || 127 < ch);
    return ch;
}

int SDL2Interface::sdl2_nh_poskey(int *x, int *y, int *mod)
{
    utf32_t ch;

    message_window->newTurn();

    // For now, accept only ASCII
    // TODO:  Accept alt-X and arrow keys
    do {
        ch = getKey(true, x, y, mod);
    } while (127 < ch);
    return ch;
}

void SDL2Interface::sdl2_nhbell(void)
{
    // TODO:  perhaps SDL_mixer or some visual indication
    fputc('\a', stdout);
}

int SDL2Interface::sdl2_doprev_message(void)
{
    message_window->prevMessage();
    return 0;
}

char SDL2Interface::sdl2_yn_function(const char *ques,
        const char *choices, char def)
{
    // Table to determine the quit character
    static struct {
        const char *choices;
        unsigned quit;
    } quit_table[] =
    {
        { "yn", 1 },
        { "ynq", 2 },
        { "ynaq", 3 },
        { "yn#aq", 4 },
        { ":ynq", 3 },
        { ":ynqm", 3 },
        { NULL, 0 }
    };

    str_context ctx = str_open_context("SDL2Interface::sdl2_yn_function");
    utf32_t ch;

    // Develop the prompt
    char prompt[BUFSZ];
    if (choices != NULL) {
        if (def != 0) {
            snprintf(prompt, SIZE(prompt), "%s [%s] (%c)", ques, choices, def);
        } else {
            snprintf(prompt, SIZE(prompt), "%s [%s]", ques, choices);
        }
    } else {
        if (def != 0) {
            snprintf(prompt, SIZE(prompt), "%s (%c)", ques, def);
        } else {
            snprintf(prompt, SIZE(prompt), "%s", ques);
        }
    }
    message_window->putStr(ATR_BOLD, prompt);
    redraw();

    if (choices == NULL) {
        // Accept any key
        do {
            ch = getKey(false);
        } while (ch <= 0 || 0x10FFFF < ch);
    } else {
        // Choices are these
        utf32_t *choices32 = uni_8to32(choices);

        // Determine the quit character
        utf32_t quitch = def;
        for (unsigned i = 0; quit_table[i].choices != NULL; ++i) {
            if (strcmp(quit_table[i].choices, choices) == 0) {
                quitch = choices32[quit_table[i].quit];
                break;
            }
        }

        // Wait for key
        do {
            ch = getKey(false);
            if (ch == '\033' && quitch != 0) {
                ch = quitch;
                break;
            } else if (ch == ' ' || ch == '\r' || ch == '\n') {
                ch = def;
                break;
            }
        } while (uni_index32(choices32, ch) == NULL);
    }

    str_close_context(ctx);
    return ch;
}

void SDL2Interface::sdl2_getlin(const char *ques, char *input)
{
    str_context ctx = str_open_context("SDL2Interface::sdl2_getlin");
    SDL2GetLine window(this);
    char *str;

    str = window.getLine(ques);
    if (str != NULL) {
        uni_copy8(input, BUFSZ, str);
    } else {
        input[0] = '\0';
    }
    str_close_context(ctx);
}

int SDL2Interface::sdl2_get_ext_cmd(void)
{
    str_context ctx = str_open_context("SDL2Interface::sdl2_get_ext_cmd");
    SDL2ExtCmd window(this);
    char *str;
    int cmd = -1;

    str = window.getLine("Enter extended command:");
    if (str != NULL) {
        for (int i = 0; extcmdlist[i].ef_txt != NULL; ++i) {
            if (strcmp(extcmdlist[i].ef_txt, str) == 0) {
                cmd = i;
                break;
            }
        }
    }

    str_close_context(ctx);
    return cmd;
}

void SDL2Interface::sdl2_number_pad(int /*state*/)
{
    // Place holder
}

void SDL2Interface::sdl2_delay_output(void)
{
    redraw();
    SDL_Delay(1);
}

void SDL2Interface::sdl2_start_screen(void)
{
    // Place holder
}

void SDL2Interface::sdl2_end_screen(void)
{
    // Place holder
}

void SDL2Interface::sdl2_outrip(winid window, int how, time_t when)
{
    // TODO:  Display the tombstone image
    genl_outrip(window, how, when);
    getKey(false);
}

void SDL2Interface::sdl2_preference_update(const char * /*pref*/)
{
    // Place holder
    // TODO:  change fonts on request
}

#ifdef USER_SOUNDS
void SDL2Interface::sdl2_play_usersound(const char *filename, int volume)
{
    // Place holder
}
#endif

nhsym chrConvert(nhsym ch)
{
    static const unsigned short cp437table[] = {
        0x0000, 0x263A, 0x263B, 0x2665, 0x2666, 0x2663, 0x2660, 0x2022,
        0x25D8, 0x25CB, 0x25D9, 0x2642, 0x2640, 0x266A, 0x266B, 0x263C,
        0x25BA, 0x25C4, 0x2195, 0x203C, 0x00B6, 0x00A7, 0x25AC, 0x21A8,
        0x2191, 0x2193, 0x2192, 0x2190, 0x221F, 0x2194, 0x25B2, 0x25BC,
        0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,
        0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F,
        0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
        0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F,
        0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
        0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,
        0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
        0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F,
        0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
        0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F,
        0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
        0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D, 0x007E, 0x2302,
        0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7,
        0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5,
        0x00C9, 0x00E6, 0x00C6, 0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9,
        0x00FF, 0x00D6, 0x00DC, 0x00A2, 0x00A3, 0x00A5, 0x20A7, 0x0192,
        0x00E1, 0x00ED, 0x00F3, 0x00FA, 0x00F1, 0x00D1, 0x00AA, 0x00BA,
        0x00BF, 0x2310, 0x00AC, 0x00BD, 0x00BC, 0x00A1, 0x00AB, 0x00BB,
        0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556,
        0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510,
        0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x255E, 0x255F,
        0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2567,
        0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256B,
        0x256A, 0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590, 0x2580,
        0x03B1, 0x00DF, 0x0393, 0x03C0, 0x03A3, 0x03C3, 0x00B5, 0x03C4,
        0x03A6, 0x0398, 0x03A9, 0x03B4, 0x221E, 0x03C6, 0x03B5, 0x2229,
        0x2261, 0x00B1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00F7, 0x2248,
        0x00B0, 0x2219, 0x00B7, 0x221A, 0x207F, 0x00B2, 0x25A0, 0x00A0,
    };
    static const unsigned short dectable[] = {
        0x25C6, 0x2592, 0x2409, 0x240C, 0x240D, 0x240A, 0x00B0, 0x00B1,
        0x2424, 0x240B, 0x2518, 0x2510, 0x250C, 0x2514, 0x253C, 0x23BA,
        0x23BB, 0x2500, 0x23BC, 0x23BD, 0x251C, 0x2524, 0x2534, 0x252C,
        0x2502, 0x2264, 0x2265, 0x03C0, 0x2260, 0x00A3, 0x00B7
    };

    if (SYMHANDLING(H_IBM)) {
        return cp437table[(unsigned char)ch];
    } else if (SYMHANDLING(H_DEC)) {
        ch &= 0xFF;
        if (0xE0 <= ch && ch <= 0xFE) {
            return dectable[ch - 0xE0];
        } else {
            return ch;
        }
    } else if (SYMHANDLING(H_UNICODE)) {
        return ch;
    } else {
        return (unsigned char)ch;
    }
}

}
