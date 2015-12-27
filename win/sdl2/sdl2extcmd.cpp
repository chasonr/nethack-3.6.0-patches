// sdl2extcmd.cpp

extern "C" {
#include "hack.h"
}
#include "unicode.h"
#include "func_tab.h"
#include "sdl2.h"
#include "sdl2extcmd.h"
#include "sdl2interface.h"
#include "sdl2font.h"

namespace NH_SDL2
{

SDL2ExtCmd::SDL2ExtCmd(SDL2Interface *interface) :
        SDL2GetLine(interface)
{
}

void SDL2ExtCmd::addChar(utf32_t ch)
{
    // Add the character the same way SDL2GetLine does
    SDL2GetLine::addChar(ch);

    // Compare to the list of extended commands; if only one match, set
    // m_contents to the match

    unsigned matches = 0;
    const char *cmd = NULL;

    for (unsigned i = 0; extcmdlist[i].ef_txt != NULL; ++i) {
        const char *text8 = extcmdlist[i].ef_txt;
        if (strncmp(text8, m_contents.c_str(), m_contents.size()) == 0) {
            ++matches;
            if (matches > 1) { break; }
            cmd = text8;
        }
    }

    if (matches == 1) {
        m_contents = cmd;
        interface()->redraw();
    }
}

}
