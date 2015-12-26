// sdl2menu.cpp

extern "C" {
#include "hack.h"
}
#include <vector>
#include "unicode.h"
#include "sdl2.h"
#include "sdl2menu.h"
#include "sdl2font.h"
#include "sdl2interface.h"

namespace NH_SDL2
{

static std::vector<std::string> tabSplit(const std::string& str);

SDL2Menu::SDL2Menu(SDL2Interface *interface) :
        SDL2Window(interface),
        m_text(false),
        m_first_line(0),
        m_page_size(0),
        m_count(0)
{
    // We won't show the window until all entries are present
    setVisible(false);

    // Menu window font
    setFont(iflags.wc_font_menu, iflags.wc_fontsiz_menu,
            SDL2Font::defaultSerifFont(), 20);
}

void SDL2Menu::clear(void)
{
    m_menu.clear();
    m_first_line = 0;
    m_prompt.clear();
    m_count = 0;
}

void SDL2Menu::redraw(void)
{
    static const SDL_Color white  = { 255, 255, 255, 255 };
    static const SDL_Color black  = {   0,   0,   0, 160 };

    interface()->fill(this, black);

    // Border around window, one pixel wide
    drawBox(0, 0, width() - 1, height() - 1, white);

    // Draw from here downwards
    int y = 1;

    if (!m_text) {
        // Display the prompt
        render(m_prompt, 1, y, textFG(ATR_NONE));
        y += lineHeight();

        // Display a border below the prompt, one pixel wide
        SDL_Rect rect = { 1, y, width() - 2, 1 };
        interface()->fill(this, rect, white);
        y += 1;
    }

    // Display the text for this page
    for (std::size_t i = 0; i < m_page_size; ++i) {
        std::size_t j = i + m_first_line;
        if (j >= m_menu.size()) { break; }
        std::vector<std::string> columns = tabSplit(m_menu[j].str);
        int attr = m_menu[j].attr;
        if (m_menu[j].identifier.a_void == NULL && columns.size() == 1) {
            // Nonselectable line with no column dividers
            // Assume this is a section header
            render(columns[0], 1, y + i*lineHeight(),
                    m_menu[j].color, textBG(attr));
        } else {
            if (m_menu[j].identifier.a_void != NULL) {
                char32_t ch = m_menu[j].ch;
                char tag[BUFSZ];
                if (ch == 0) { ch = m_menu[j].def_ch; }
                snprintf(tag, SIZE(tag), "%1$c %2$c - ",
                        !m_menu[j].selected ? ' ' : m_menu[j].count == 0 ? '*' : '#',
                        ch);
                render(tag, 1, y + i*lineHeight(),
                        m_menu[j].color, textBG(attr));
            } else {
                if (columns[0].substr(0, 6) == "      ") {
                    columns[0] = columns[0].substr(6);
                }
            }
            for (std::size_t k = 0; k < columns.size(); ++k) {
                render(columns[k],
                        m_columns[k].x, y + i*lineHeight(),
                        m_menu[j].color, textBG(attr));
            }
        }
    }

    if (m_page_size < m_menu.size()) {
        // Display the page indicator
        char page[BUFSZ];
        snprintf(page, SIZE(page), "Page %u of %u",
                (unsigned) (m_first_line / m_page_size + 1),
                (unsigned) ((m_menu.size() + m_page_size - 1) / m_page_size));
        render(page, 1, y + m_page_size*lineHeight(),
                textFG(ATR_NONE), textBG(ATR_NONE));
    }
}

void SDL2Menu::startMenu(void)
{
    m_text = false;
}

void SDL2Menu::addMenu(int glyph, const anything* identifier, char ch,
        char gch, int attr, const std::string& str, bool preselected)
{
    MenuEntry entry;

    // Add the entry to m_menu
    entry.glyph = glyph;
    entry.identifier = *identifier;
    entry.ch = ch;
    entry.gch = gch;
    entry.def_ch = 0;
    entry.attr = attr;
    entry.str = str;
    entry.selected = preselected;
    // TODO: implement menucolors here
    entry.color = textFG(attr);
    entry.count = 0;

    m_menu.push_back(entry);
}

void SDL2Menu::endMenu(const std::string& prompt)
{
    m_prompt = prompt;
}

int SDL2Menu::selectMenu(int how, menu_item ** menu_list)
{
    char32_t ch;
    int count;

    // Now show the window
    setWindowSize();
    setVisible(true);
    interface()->redraw();

    // Read keys and select accordingly
    m_count = 0;
    do {
        ch = interface()->getKey();
        if (selectEntry(ch, how) && how == PICK_ONE) { break; }
        doPage(ch);
    } while (ch != '\033' && ch != '\r');

    *menu_list = NULL;
    count = 0;
    if (ch == '\033') {
        count = -1;
    } else if (how == PICK_ONE) {
        *menu_list = NULL;
        count = 0;
        for (std::vector<MenuEntry>::iterator p = m_menu.begin();
                p != m_menu.end(); ++p) {
            if (p->selected) {
                *menu_list = (menu_item *) alloc(sizeof(menu_item) * 1);
                (*menu_list)[0].item = p->identifier;
                (*menu_list)[0].count = p->count ? p->count : -1;
                count = 1;
                break;
            }
        }
    } else if (how == PICK_ANY) {
        int i;

        for (std::vector<MenuEntry>::iterator p = m_menu.begin();
                p != m_menu.end(); ++p) {
            if (p->selected) {
                ++count;
            }
        }
        if (count != 0) {
            *menu_list = (menu_item *) alloc(sizeof(menu_item) * count);
            i = 0;
            for (std::vector<MenuEntry>::iterator p = m_menu.begin();
                    p != m_menu.end(); ++p) {
                if (p->selected) {
                    (*menu_list)[i].item = p->identifier;
                    (*menu_list)[i].count = p->count ? p->count : -1;
                    ++i;
                }
            }
        }
    }
    setVisible(false);
    clear();
    return count;
}

// For text windows
void SDL2Menu::putStr(int attr, const std::string& str)
{
    m_text = true;

    MenuEntry entry;

    // Add the entry to m_menu
    entry.glyph = 0;
    entry.identifier.a_void = NULL;
    entry.ch = 0;
    entry.gch = 0;
    entry.def_ch = 0;
    entry.attr = attr;
    entry.str = str;
    entry.selected = false;
    entry.color.r = 176;
    entry.color.g = 176;
    entry.color.b = 176;
    entry.color.a = 255;
    entry.count = 0;

    m_menu.push_back(entry);
}

void SDL2Menu::setVisible(bool visible)
{
    setWindowSize();
    SDL2Window::setVisible(visible);
    if (m_text) {
        char32_t ch;
        do {
            interface()->redraw();
            ch = interface()->getKey();
            doPage(ch);
        } while (ch != '\033' && ch != '\r');
    }
}

void SDL2Menu::setWindowSize(void)
{
    static const char letters[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int w, h;
#if 0
    SDL_Rect rect;
#endif

    if (font() == NULL) { return; } // Not completely set up

    // Height is the sum of all line heights, plus two for borders
    h = m_menu.size() * lineHeight() + 2;
    // ...plus one more border and one more line if this is a menu
    if (!m_text) {
        h += lineHeight() + 1;
    }

    // Width is the greatest width of any line, plus two for borders
    w = buildColumnTable();

    w += 2;

    // Add this to the width for a right margin
    w += (font()->textSize("  ")).w;

    if (h <= interface()->height()) {
        // The height fits within the main window
        m_page_size = m_menu.size();
        if (m_page_size > 52) { m_page_size = 52; }
    } else if (m_text) {
        // We need to break the menu into pages
        m_page_size = (interface()->height() - 2) / lineHeight() - 1;
        if (m_page_size < 1) { m_page_size = 1; }
        h = (m_page_size + 1) * lineHeight() + 3;
    } else {
        // We need to break the menu into pages
        m_page_size = (interface()->height() - 3) / lineHeight() - 2;
        if (m_page_size < 1) { m_page_size = 1; }
        if (m_page_size > 52) { m_page_size = 52; }
        h = (m_page_size + 2) * lineHeight() + 3;
    }

    int x0 = (interface()->width() - w) / 2;
    int y0 = (interface()->height() - h) / 2;
    resize(x0, y0, x0 + w - 1, y0 + h - 1);

    // Assign default letters to entries without a letter
    for (std::size_t i = 0; i < m_menu.size(); i += m_page_size) {
        unsigned l = 0;
        for (std::size_t j = 0; j < m_page_size; ++j) {
            if (i+j >= m_menu.size()) { break; }
            if (m_menu[i+j].identifier.a_void != NULL) {
                // l never overflows because the page size is limited to 52
                m_menu[i+j].def_ch = letters[l++];
            }
        }
    }
}

int SDL2Menu::buildColumnTable(void)
{
    int width;

    if (m_text) {
        width = 0;
    } else {
        width = (font()->textSize(m_prompt)).w;
    }

    // Clear column table
    m_columns.clear();

    // Add and extend columns
    for (std::vector<MenuEntry>::iterator p = m_menu.begin();
            p != m_menu.end(); ++p) {
        std::vector<std::string> columns = tabSplit(p->str);
        if (p->identifier.a_void == NULL && columns.size() == 1) {
            // Nonselectable line with no column dividers
            // Assume this is a section header
            int w2 = (font()->textSize(p->str)).w;
            if (width < w2) {
                width = w2;
            }
        } else {
            // Extend m_columns to have at least as many columns as the line
            while (m_columns.size() < columns.size()) {
                static const MenuColumn zero = { 0, 0 };
                m_columns.push_back(zero);
            }
            // Widen each column to fit the line
            for (std::size_t i = 0; i < columns.size(); ++i) {
                int w2 = (font()->textSize(columns[i])).w;
                if (m_columns[i].width < w2) {
                    m_columns[i].width = w2;
                }
            }
        }
    }

    if (m_columns.empty()) {
        return width;
    }

    // Place the left edge of each column
    int pad = (font()->textSize("  ")).w;
    if (!m_text) {
        m_columns[0].x = font()->textSize("# A -").w;
    }
    for (std::size_t i = 1; i < m_columns.size(); ++i) {
        m_columns[i].x = m_columns[i-1].x + m_columns[i-1].width + pad;
    }

    // Extend overall width for widest columned line
    int w2 = m_columns[m_columns.size() - 1].x
           + m_columns[m_columns.size() - 1].width
           + pad;
    if (width < w2) {
        width = w2;
    }

    // Return overall width
    return width;
}

bool SDL2Menu::selectEntry(char32_t ch, int how)
{
    if (how == PICK_NONE) { return false; }

    // Process count
    if ('0' <= ch && ch <= '9') {
        unsigned long count2;
        unsigned digit;

        count2 = m_count * 10;
        if (count2 / 10 != m_count) { return false; } // overflow
        digit = ch - '0';
        count2 += digit;
        if (count2 >= digit) {
            m_count = count2;
        }
        return false;
    }

    if (how == PICK_ANY) {
        switch (ch) {
        case MENU_SELECT_ALL:
            m_count = 0;
            for (std::vector<MenuEntry>::iterator p = m_menu.begin();
                    p != m_menu.end(); ++p) {
                if (p->identifier.a_void == NULL) { continue; }
                p->selected = true;
                p->count = 0;
            }
            interface()->redraw();
            return true;

        case MENU_UNSELECT_ALL:
            m_count = 0;
            for (std::vector<MenuEntry>::iterator p = m_menu.begin();
                    p != m_menu.end(); ++p) {
                if (p->identifier.a_void == NULL) { continue; }
                p->selected = false;
                p->count = 0;
            }
            interface()->redraw();
            return true;

        case MENU_INVERT_ALL:
            m_count = 0;
            for (std::vector<MenuEntry>::iterator p = m_menu.begin();
                    p != m_menu.end(); ++p) {
                if (p->identifier.a_void == NULL) { continue; }
                p->selected = !p->selected;
                p->count = 0;
            }
            interface()->redraw();
            return true;

        case MENU_SELECT_PAGE:
            m_count = 0;
            for (std::size_t i = 0; i < m_page_size; ++i) {
                std::size_t j = i + m_first_line;
                if (j >= m_menu.size()) { break; }
                if (m_menu[j].identifier.a_void == NULL) { continue; }
                m_menu[j].selected = true;
                m_menu[j].count = 0;
            }
            interface()->redraw();
            return true;

        case MENU_UNSELECT_PAGE:
            m_count = 0;
            for (std::size_t i = 0; i < m_page_size; ++i) {
                std::size_t j = i + m_first_line;
                if (j >= m_menu.size()) { break; }
                if (m_menu[j].identifier.a_void == NULL) { continue; }
                m_menu[j].selected = false;
                m_menu[j].count = 0;
            }
            interface()->redraw();
            return true;

        case MENU_INVERT_PAGE:
            m_count = 0;
            for (std::size_t i = 0; i < m_page_size; ++i) {
                std::size_t j = i + m_first_line;
                if (j >= m_menu.size()) { break; }
                if (m_menu[j].identifier.a_void == NULL) { continue; }
                m_menu[j].selected = !m_menu[i].selected;
                m_menu[j].count = 0;
            }
            interface()->redraw();
            return true;
        }
    }

    bool selected = false;

    for (std::size_t i = 0; i < m_menu.size(); ++i) {
        if (m_menu[i].identifier.a_void == NULL) { continue; }
        if (ch == m_menu[i].ch || ch == m_menu[i].gch
        ||  (m_menu[i].ch == 0 && ch == m_menu[i].def_ch && m_first_line <= i && i < m_first_line + m_page_size)) {
            if (m_count != 0) {
                m_menu[i].selected = true;
                m_menu[i].count = m_count;
            } else {
                m_menu[i].selected = !m_menu[i].selected;
                m_menu[i].count = 0;
            }
            selected = true;
        }
    }

    if (selected) {
        m_count = 0;
        interface()->redraw();
    }

    return selected;
}

void SDL2Menu::doPage(char32_t ch)
{
    switch (ch) {
    case MENU_FIRST_PAGE:
        if (m_first_line != 0) {
            m_first_line = 0;
            interface()->redraw();
        }
        break;

    case MENU_LAST_PAGE:
        if (m_page_size < m_menu.size()) {
            int pagenum = (m_menu.size() - 1) / m_page_size;
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
        if (m_first_line + m_page_size < m_menu.size()) {
            m_first_line += m_page_size;
            interface()->redraw();
        }
        break;
    }
}

static std::vector<std::string>
tabSplit(const std::string& str)
{
    std::size_t i, j;
    std::vector<std::string> columns;

    i = 0;
    do {
        j = str.find('\t', i);
        if (j == std::string::npos) {
            j = str.size();
        }
        columns.push_back(str.substr(i, j - i));
        i = j + 1;
    } while (i < str.size());

    return columns;
}

}
