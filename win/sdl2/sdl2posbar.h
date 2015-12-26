// sdl2posbar.h

#ifndef SDL2POSBAR_H
#define SDL2POSBAR_H

#include <string>
#include "sdl2.h"
#include "sdl2window.h"

namespace NH_SDL2
{

#ifdef POSITIONBAR
class SDL2Interface;
class SDL2MapWindow;

class SDL2PositionBar : public SDL2Window
{
public:
    SDL2PositionBar(SDL2Interface *interface, SDL2MapWindow *map_window);

    virtual void redraw(void);

    int heightHint(void);

    void updatePositionBar(const char *data);

private:
    int m_text_w, m_text_h;
    std::string m_data;
    SDL2MapWindow *m_map_window;
};
#endif

}

#endif
