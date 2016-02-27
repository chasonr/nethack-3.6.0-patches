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
    m_contents(NULL),
    m_num_lines(0),
    m_lines_alloc(0),
    m_first_line(0),
    m_page_size(0)
{
    // We won't show the window until all lines are present
    setVisible(false);

    // Text window font
    setFont(iflags.wc_font_text, iflags.wc_fontsiz_text,
            SDL2Font::defaultSerifFont(), 20);
}

SDL2Text::~SDL2Text(void)
{
    size_t i;

    for (i = 0; i < m_num_lines; ++i) {
        delete[] m_contents[i].text;
    }
    delete[] m_contents;
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
        if (j >= m_num_lines) { break; }
        int attr = m_contents[j].attributes;
        render(m_contents[j].text, margin, y, textFG(attr), textBG(attr));
        y += lineHeight();
    }
    if (m_page_size < m_num_lines) {
        // Display the page indicator
        y = height() - lineHeight() - margin;
        char page[QBUFSZ];
        snprintf(page, SIZE(page), "Page %u of %u",
                (unsigned) (m_first_line / m_page_size + 1),
                (unsigned) ((m_num_lines + m_page_size - 1) / m_page_size));
        render(page, margin, y, textFG(ATR_NONE), textBG(ATR_NONE));
    }
}

void SDL2Text::clear(void)
{
    size_t i;

    for (i = 0; i < m_num_lines; ++i) {
        delete[] m_contents[i].text;
    }
    m_num_lines = 0;
}

void SDL2Text::putStr(int attr, const char *str)
{
    Line *new_line;

    expandText();
    new_line = &m_contents[m_num_lines++];
    new_line->text = new char[strlen(str) + 1];
    strcpy(new_line->text, str);
    new_line->attributes = attr;
}

void SDL2Text::setVisible(bool visible)
{
    if (visible) {
        int width, height;

        width = 0;
        for (size_t i = 0; i < m_num_lines; ++i) {
            SDL_Rect rect;

            rect = font()->textSize(m_contents[i].text);
            if (width < rect.w) {
                width = rect.w;
            }
        }

        height = m_num_lines * lineHeight();
        if (height + margin * 2 > interface()->height()) {
            m_page_size = (interface()->height() - margin * 2) / lineHeight() - 1;
            if (m_page_size < 1) { m_page_size = 1; }
            height = (m_page_size + 1) * lineHeight();
        } else {
            m_page_size = m_num_lines;
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
        utf32_t ch;
        do {
            ch = interface()->getKey();
            doPage(ch);
        } while (ch != '\033' && ch != '\r');
    }
}

void SDL2Text::doPage(utf32_t ch)
{
    switch (ch) {
    case MENU_FIRST_PAGE:
        if (m_first_line != 0) {
            m_first_line = 0;
            interface()->redraw();
        }
        break;

    case MENU_LAST_PAGE:
        if (m_page_size < m_num_lines) {
            int pagenum = (m_num_lines - 1) / m_page_size;
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
        if (m_first_line + m_page_size < m_num_lines) {
            m_first_line += m_page_size;
            interface()->redraw();
        }
        break;
    }
}

void SDL2Text::expandText(void)
{
    if (m_num_lines >= m_lines_alloc) {
        Line *new_contents;
        size_t i;

        m_lines_alloc += 64;
        new_contents = new Line[m_lines_alloc];
        for (i = 0; i < m_num_lines; ++i) {
            new_contents[i] = m_contents[i];
        }
        delete[] m_contents;
        m_contents = new_contents;
    }
}

}
