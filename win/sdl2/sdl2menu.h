// sdl2menu.h

#ifndef SDL2MENU_H
#define SDL2MENU_H

#include <cstddef>
#include "sdl2window.h"

namespace NH_SDL2
{

class SDL2Interface;

class SDL2Menu : public SDL2Window
{
public:
    SDL2Menu(SDL2Interface *interface);
    ~SDL2Menu(void);

    virtual void redraw(void);

    virtual void clear(void);
    virtual void startMenu(void);
    virtual void addMenu(int glyph, const anything* identifier, char ch,
            char gch, int attr, const char *str, bool preselected);
    virtual void endMenu(const char *prompt);
    virtual int  selectMenu(int how, menu_item ** menu_list);
    virtual void putStr(int attr, const char *str);
    virtual void setVisible(bool visible);

private:
    struct MenuEntry
    {
        int glyph;
        anything identifier;
        utf32_t ch;
        utf32_t gch;
        utf32_t def_ch;
        int attr;
        char *str;
        bool selected;
        SDL_Color color;
        unsigned long count;
    };

    bool m_text;
    MenuEntry *m_menu;
    std::size_t m_menu_size;
    std::size_t m_menu_alloc;
    std::size_t m_first_line;
    char *m_prompt;
    int m_page_size;
    unsigned long m_count;

    void setWindowSize(void);
    bool selectEntry(utf32_t ch, int hoh);
    void doPage(utf32_t ch);

    struct MenuColumn
    {
        int x;
        int width;
    };

    MenuColumn *m_columns;
    std::size_t m_num_columns;
    int buildColumnTable(void);
    void expandMenu(void);
};

}

#endif
