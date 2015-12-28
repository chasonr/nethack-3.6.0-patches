// sdl2text.h

#ifndef SDL2TEXT_H
#define SDL2TEXT_H

#include "sdl2window.h"

namespace NH_SDL2
{

class SDL2Interface;

class SDL2Text : public SDL2Window
{
public:
    SDL2Text(SDL2Interface *interface);
    ~SDL2Text(void);

    virtual void redraw(void);

    virtual void clear(void);
    virtual void putStr(int attr, const char *str);
    virtual void setVisible(bool visible);

private:
    struct Line
    {
        char *text;
        uint32_t attributes;
    };

    Line *m_contents;
    size_t m_num_lines;
    size_t m_lines_alloc;
    unsigned m_first_line;
    unsigned m_page_size;

    void doPage(utf32_t ch);
    void expandText(void);
};

}

#endif
