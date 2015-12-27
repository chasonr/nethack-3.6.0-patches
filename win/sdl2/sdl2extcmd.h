// sdl2extcmd.h

#ifndef SDL2EXTCMD_H
#define SDL2EXTCMD_H

#include "sdl2getlin.h"

namespace NH_SDL2
{

class SDL2ExtCmd : public SDL2GetLine
{
public:
    SDL2ExtCmd(SDL2Interface *interface);
    virtual void addChar(utf32_t ch);
};

}

#endif
