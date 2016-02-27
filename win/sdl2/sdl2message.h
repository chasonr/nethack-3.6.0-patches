// sdl2message.h

#ifndef SDL2MESSAGE_H
#define SDL2MESSAGE_H

#include "sdl2.h"
#include "sdl2window.h"

namespace NH_SDL2
{

class SDL2Interface;

class SDL2MessageWindow : public SDL2Window
{
public:
    SDL2MessageWindow(SDL2Interface *interface);
    ~SDL2MessageWindow(void);

    virtual void redraw(void);

    int heightHint(void);

    virtual void putStr(int attr, const char *str);
    virtual void putMixed(int attr, const char *str);
    void prevMessage(void);

    void newTurn(void) { m_combine = false; }
    void more(void);

private:
    struct Line
    {
        int glyph;
        char *text;
        uint32_t attributes;
        unsigned num_messages;
    };

    struct LineList
    {
        Line *lines;
        size_t size;
        size_t head;
        size_t tail;
    };

    LineList m_contents; // Uncombined messages
    LineList m_lines; // Messages combined into lines

    int m_line_offset;
    int m_more_width;
    bool m_more;
    bool m_combine;

    void putMixed(int attr, const char *str, int glyph);

    static void initLineList(LineList *list, size_t size);
    static void freeLineList(LineList *list);
    static size_t numberOfLines(const LineList *list);
    static Line *addLine(LineList *list, int attr, const char *str, int glyph);
};

}

#endif
