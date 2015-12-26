// sdl2getlin.cpp

extern "C" {
#include "hack.h"
}
#include "unicode.h"
#include "sdl2.h"
#include "sdl2getlin.h"
#include "sdl2interface.h"
#include "sdl2font.h"

namespace NH_SDL2
{

const int margin = 3;
const int min_width = 400;

SDL2GetLine::SDL2GetLine(SDL2Interface *interface) :
    SDL2Window(interface),
    m_box_width(0)
{
    setVisible(false);

    // Message window font
    setFont(iflags.wc_font_message, iflags.wc_fontsiz_message,
            SDL2Font::defaultSerifFont(), 20);
}

void SDL2GetLine::redraw(void)
{
    const SDL_Color black = {   0,   0,   0, 255 };
    const SDL_Color white = { 255, 255, 255, 255 };
    SDL_Rect rect;
    SDL_Surface *text;

    interface()->fill(this, black);

    // Draw a border at the edges of the window
    drawBox(0, 0, width()-1, height()-1, white);

    // Draw a border around the text being entered
    drawBox(margin, 2*margin + lineHeight(),
            width() - 1 - margin, height() - 1 - margin,
            white);

    // Draw the prompt
    render(m_prompt, margin, margin, white, black);

    // Draw the text as it currently stands
    SDL_Rect source;
    text = font()->render(m_contents, white, black);
    if (text->w < m_box_width) {
        // The text fits within the box width
        source.w = text->w;
    } else {
        // Clip the text at the left
        source.x = text->w - (m_box_width - 1);
    }
    source.x = text->w - source.w;
    source.y = 0;
    source.h = text->h;
    rect.x = 3*margin;
    rect.y = 3*margin + lineHeight();
    rect.w = source.w;
    rect.h = source.h;
    interface()->blit(this, rect, text, source);
    SDL_FreeSurface(text);

    // Draw a cursor
    rect.x += rect.w;
    rect.w = 1;
    rect.h = lineHeight();
    interface()->fill(this, rect, white);
}


bool SDL2GetLine::getLine(const std::string& prompt, std::string& line)
{
    StringContext ctx("SDL2GetLine::getLine");

    // Set the prompt
    m_prompt = prompt;

    // Height is a margin, a line, two margins, a line and two margins
    int h = margin * 5 + lineHeight() * 2;

    // Width is a margin, the longer of the prompt or min_width, and a margin
    int w = (font()->textSize(prompt)).w;
    if (w < min_width) { w = min_width; }
    w += margin * 2;

    // Width is two margins, the box width and two margins
    m_box_width = w - margin * 4;

    // Center the window
    int x0 = (interface()->width() - w) / 2;
    int y0 = (interface()->height() - h) / 2;
    resize(x0, y0, x0 + w - 1, y0 + h - 1);

    // Make it visible
    setVisible(true);
    interface()->redraw();

    // Accept keys
    while (true) {
        char32_t ch = interface()->getKey();
        if (ch == '\033') { return false; } // escape
        if (ch == '\n' || ch == '\r') { break; }
        addChar(ch);
    }

    // Returned false above if escape

    line = std::string(uni_normalize8(m_contents.c_str(), "NFC"));
    return true;
}

void SDL2GetLine::addChar(char32_t ch)
{
    // TODO:  process left and right arrows
    if (ch > 0x10FFFF) { return; }

    if ((ch == '\b' || ch == '\177') && !m_contents.empty()) {
        m_contents = m_contents.substr(0, m_contents.size() - 1);
    } else {
        // Don't include control characters in the string
        if (ch < 0x20 || (0x7F <= ch && ch <= 0x9F)) { return; }

        // Add this character
        m_contents += ch;
    }

    interface()->redraw();
}

}
