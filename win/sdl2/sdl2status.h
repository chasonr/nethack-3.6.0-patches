// sdl2status.h

#ifndef SDL2STATUS_H
#define SDL2STATUS_H

#include <string>
#include "sdl2.h"
#include "sdl2window.h"

namespace NH_SDL2
{

class SDL2Interface;

class SDL2StatusWindow : public SDL2Window
{
public:
    SDL2StatusWindow(SDL2Interface *interface);

    virtual void redraw(void);

    int heightHint(void);

    virtual void putStr(int attr, const std::string& str);
    virtual void clear(void);
    virtual void setCursor(int x, int y);
};

}

#endif
