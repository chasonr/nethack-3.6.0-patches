// sdl2getlin.h

#ifndef SDL2GETLIN_H
#define SDL2GETLIN_H

#include <string>
#include "sdl2window.h"

namespace NH_SDL2
{

class SDL2Interface;

class SDL2GetLine : public SDL2Window
{
public:
    SDL2GetLine(SDL2Interface *interface);

    virtual void redraw(void);

    bool getLine(const std::string& prompt, std::string& line);

protected:
    // SDL2ExtCmd overrides this for extended command processing
    virtual void addChar(utf32_t ch);

    // SDL2ExtCmd modifies this
    std::string m_contents;

private:
    int m_box_width;

    std::string m_prompt;
};

}

#endif
