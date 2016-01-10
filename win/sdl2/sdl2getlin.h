// sdl2getlin.h

#ifndef SDL2GETLIN_H
#define SDL2GETLIN_H

#include "sdl2window.h"

namespace NH_SDL2
{

class SDL2Interface;

class SDL2GetLine : public SDL2Window
{
public:
    SDL2GetLine(SDL2Interface *interface);
    virtual ~SDL2GetLine(void);

    virtual void redraw(void);

    char *getLine(const char *prompt);

protected:
    // SDL2ExtCmd overrides this for extended command processing
    virtual void addChar(utf32_t ch);

    // SDL2ExtCmd modifies this
    char *m_contents;
    size_t m_contents_size;
    size_t m_contents_alloc;

private:
    int m_box_width;

    char *m_prompt;
};

}

#endif
