// sdl2menu.h

#ifndef SDL2MENU_H
#define SDL2MENU_H

#include <string>
#include <vector>
#include "sdl2window.h"

namespace NH_SDL2
{

class SDL2Interface;

class SDL2Menu : public SDL2Window
{
public:
    SDL2Menu(SDL2Interface *interface);

    virtual void redraw(void);

    virtual void clear(void);
    virtual void startMenu(void);
    virtual void addMenu(int glyph, const anything* identifier, char ch,
            char gch, int attr, const std::string& str, bool preselected);
    virtual void endMenu(const std::string& prompt);
    virtual int  selectMenu(int how, menu_item ** menu_list);
    virtual void putStr(int attr, const std::string& str);
    virtual void setVisible(bool visible);

private:
    struct MenuEntry
    {
        int glyph;
        anything identifier;
        char32_t ch;
        char32_t gch;
        char32_t def_ch;
        int attr;
        std::string str;
        bool selected;
        SDL_Color color;
        unsigned long count;
    };

    bool m_text;
    std::vector<MenuEntry> m_menu;
    std::size_t m_first_line;
    std::string m_prompt;
    int m_page_size;
    unsigned long m_count;

    void setWindowSize(void);
    bool selectEntry(char32_t ch, int hoh);
    void doPage(char32_t ch);

    struct MenuColumn
    {
        int x;
        int width;
    };

    std::vector<MenuColumn> m_columns;
    int buildColumnTable(void);
};

}

#endif
