// sdl2window.h

#ifndef SDL2WINDOW_H
#define SDL2WINDOW_H

class SDL2Font;

namespace NH_SDL2
{

class SDL2Interface;

class SDL2Window
{
public:
    SDL2Window(SDL2Interface *interface);
    virtual ~SDL2Window(void);

    SDL2Interface *interface(void) { return m_interface; }

    // Sizing of window
    virtual void resize(int x1, int y1, int x2, int y2);
    int xmin(void) { return m_xmin; }
    int xmax(void) { return m_xmax; }
    int ymin(void) { return m_ymin; }
    int ymax(void) { return m_ymax; }
    int width(void) { return m_xmax - m_xmin + 1; }
    int height(void) { return m_ymax - m_ymin + 1; }

    // Drawing of window
    virtual void redraw(void);
    bool isVisible(void) { return m_visible; }
    virtual void setVisible(bool visible);

    void drawBox(int x1, int y1, int x2, int y2, SDL_Color color);

    // Keyboard events to window
    // TBD

    // Mouse events to window
    // TBD

    virtual void startMenu(void);
    virtual void addMenu(int glyph, const anything* identifier, char ch,
            char gch, int attr, const char *str, bool preselected);
    virtual void endMenu(const char *prompt);
    virtual int  selectMenu(int how, menu_item ** menu_list);
    virtual void putStr(int attr, const char *str);
    virtual void putMixed(int attr, const char *str);
    virtual void clear(void);
    virtual void printGlyph(xchar x, xchar y, int glyph, int bkglyph);
    virtual void setCursor(int x, int y);

protected:
    // For rendering of text
    void setFont(const char *nameOption, int sizeOption,
                 const char *defaultName, int defaultSize);
    SDL2Font *font(void) { return m_font; }
    int lineHeight(void) { return m_line_height; }

    SDL_Rect render(
            const char *str,
            int x, int y,
            SDL_Color fg);
    SDL_Rect render(
            const char *str,
            int x, int y,
            SDL_Color fg, SDL_Color bg);

    static SDL_Color textFG(int attr);
    static SDL_Color textBG(int attr);
    static SDL_Color textColor(uint32_t color);

private:
    SDL2Interface *m_interface;
    int m_xmin;
    int m_xmax;
    int m_ymin;
    int m_ymax;
    int m_visible;

    SDL2Font *m_font;
    int m_line_height;
};

}

#endif
