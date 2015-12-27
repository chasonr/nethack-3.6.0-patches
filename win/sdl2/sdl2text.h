// sdl2text.h

#ifndef SDL2TEXT_H
#define SDL2TEXT_H

#include <vector>
#include <string>
#include "sdl2window.h"

namespace NH_SDL2
{

class SDL2Interface;

class SDL2Text : public SDL2Window
{
public:
    SDL2Text(SDL2Interface *interface);

    virtual void redraw(void);

    virtual void clear(void);
    virtual void putStr(int attr, const std::string& str);
    virtual void setVisible(bool visible);

private:
    struct Line
    {
        std::string text;
        uint32_t attributes;
    };

    std::vector<Line> m_contents;
    unsigned m_first_line;
    unsigned m_page_size;

    void doPage(utf32_t ch);
};

}

#endif
