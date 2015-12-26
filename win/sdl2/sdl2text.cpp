// sdl2text.cpp

extern "C" {
#include "hack.h"
}
#include "sdl2.h"
#include "sdl2text.h"
#include "sdl2font.h"
#include "sdl2interface.h"

namespace NH_SDL2
{

const int margin = 3;

SDL2Text::SDL2Text(SDL2Interface *interface) :
    SDL2Window(interface),
    m_first_line(0),
    m_page_size(0)
{
    // We won't show the window until all lines are present
    setVisible(false);

    // Text window font
    setFont(iflags.wc_font_text, iflags.wc_fontsiz_text,
            SDL2Font::defaultSerifFont(), 20);
}

void SDL2Text::redraw(void)
{
    static const SDL_Color white = { 255, 255, 255, 255 };
    static const SDL_Color black = {   0,   0,   0, 160 };
    int y;

    interface()->fill(this, black);

    // Border around window, one pixel wide
    drawBox(0, 0, width() - 1, height() - 1, white);

    y = margin;
    for (unsigned i = 0; i < m_page_size; ++i) {
        if (y >= height()) { break; }
        unsigned j = i + m_first_line;
        if (j >= m_contents.size()) { break; }
        int attr = m_contents[j].attributes;
        render(m_contents[j].text, margin, y, textFG(attr), textBG(attr));
        y += lineHeight();
    }
    if (m_page_size < m_contents.size()) {
        // Display the page indicator
        y = height() - lineHeight() - margin;
        char page[QBUFSZ];
        snprintf(page, SIZE(page), "Page %u of %u",
                (unsigned) (m_first_line / m_page_size + 1),
                (unsigned) ((m_contents.size() + m_page_size - 1) / m_page_size));
        render(page, margin, y, textFG(ATR_NONE), textBG(ATR_NONE));
    }
}

void SDL2Text::clear(void)
{
    m_contents.clear();
}

void SDL2Text::putStr(int attr, const std::string& str)
{
    Line new_line;
    new_line.text = str;
    new_line.attributes = attr;
    m_contents.push_back(new_line);
}

void SDL2Text::setVisible(bool visible)
{
    if (visible) {
        int width, height;

        width = 0;
        for (std::vector<Line>::iterator p = m_contents.begin();
                p != m_contents.end(); ++p) {
            SDL_Rect rect;

            rect = font()->textSize(p->text);
            if (width < rect.w) {
                width = rect.w;
            }
        }

        height = m_contents.size() * lineHeight();
        if (height + margin * 2 > interface()->height()) {
            m_page_size = (interface()->height() - margin * 2) / lineHeight() - 1;
            if (m_page_size < 1) { m_page_size = 1; }
            height = (m_page_size + 1) * lineHeight();
        } else {
            m_page_size = m_contents.size();
        }
        width += margin * 2;
        height += margin * 2;

        int x, y;

        x = (interface()->width() - width) / 2;
        y = (interface()->height() - height) / 2;
        resize(x, y, x + width - 1, y + height - 1);
    }

    SDL2Window::setVisible(visible);
    interface()->redraw();

    if (visible) {
        char32_t ch;
        do {
            ch = interface()->getKey();
            doPage(ch);
        } while (ch != '\033' && ch != '\r');
    }
}

void SDL2Text::doPage(char32_t ch)
{
    switch (ch) {
    case MENU_FIRST_PAGE:
        if (m_first_line != 0) {
            m_first_line = 0;
            interface()->redraw();
        }
        break;

    case MENU_LAST_PAGE:
        if (m_page_size < m_contents.size()) {
            int pagenum = (m_contents.size() - 1) / m_page_size;
            int first_line = pagenum * m_page_size;
            if (m_first_line != first_line) {
                m_first_line = first_line;
                interface()->redraw();
            }
        }
        break;

    case MENU_PREVIOUS_PAGE:
        if (m_first_line >= m_page_size) {
            m_first_line -= m_page_size;
            interface()->redraw();
        }
        break;

    case MENU_NEXT_PAGE:
        if (m_first_line + m_page_size < m_contents.size()) {
            m_first_line += m_page_size;
            interface()->redraw();
        }
        break;
    }
}

}
