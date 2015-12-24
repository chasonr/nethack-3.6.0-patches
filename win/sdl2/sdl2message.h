// sdl2message.h

#ifndef SDL2MESSAGE_H
#define SDL2MESSAGE_H

#include <list>
#include <string>
#include "sdl2.h"
#include "sdl2window.h"

namespace NH_SDL2
{

class SDL2Interface;

class SDL2MessageWindow : public SDL2Window
{
public:
    SDL2MessageWindow(SDL2Interface *interface);

    virtual void redraw(void);

    int heightHint(void);

    virtual void putStr(int attr, const std::string& str);
    void prevMessage(void);

    void newTurn(void) { m_combine = false; }
    void more(void);

private:
    struct Line
    {
        std::string text;
        uint32_t attributes;
    };

    std::list<Line> m_contents; // Uncombined messages

    struct CombinedLine : public Line
    {
        unsigned num_messages;
    };

    std::list<CombinedLine> m_lines; // Messages combined into lines

    int m_line_offset;
    int m_more_width;
    bool m_more;
    bool m_combine;
};

}

#endif
