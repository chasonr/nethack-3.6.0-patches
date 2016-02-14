// sdl2message.cpp

extern "C" {
#include "hack.h"
}
#include "sdl2.h"
#include "sdl2message.h"
#include "sdl2interface.h"
#include "sdl2font.h"
#include "sdl2map.h"

namespace NH_SDL2
{

SDL2MessageWindow::SDL2MessageWindow(SDL2Interface *interface) :
    SDL2Window(interface),
    m_line_offset(0),
    m_more_width(0),
    m_more(false),
    m_combine(false)
{
    // Message window font
    setFont(iflags.wc_font_message, iflags.wc_fontsiz_message,
            SDL2Font::defaultSerifFont(), 20);

    // Width of --More--
    m_more_width = (font()->textSize("--More--")).w;

    initLineList(&m_contents, iflags.msg_history + 1);
    initLineList(&m_lines, iflags.msg_history + 1);
}

SDL2MessageWindow::~SDL2MessageWindow(void)
{
    freeLineList(&m_contents);
    freeLineList(&m_lines);
}

void SDL2MessageWindow::redraw(void)
{
    const SDL_Color background = {   0,   0,   0, 255 };
    StringContext ctx("SDL2MessageWindow::redraw");
    int num_lines;  // Number of visible lines
    int i;
    SDL_Rect rect;

    num_lines = (height() + lineHeight() - 1) / lineHeight();
    size_t p = m_lines.tail;
    if (m_lines.head > m_lines.tail) {
        p += m_lines.size;
    }
    if (p - m_lines.head < m_line_offset) {
        p = m_lines.head;
    } else {
        p -= m_line_offset;
        if (p >= m_lines.size) {
            p -= m_lines.size;
        }
    }
    for (i = 0; i < num_lines && p != m_lines.head; ++i) {
        if (p == 0) {
            p = m_lines.size;
        }
        --p;
        int attr = m_lines.lines[p].attributes;
        int x = 0;
        int y = height() - lineHeight()*(i+1);
        if (m_lines.lines[p].glyph != NO_GLYPH) {
            nhsym ch;
            int oc;
            unsigned os;
            utf32_t ch32[2];
            mapglyph(m_lines.lines[p].glyph, &ch, &oc, &os, 0, 0);
            ch32[0] = chrConvert(ch);
            ch32[1] = 0;
            char *utf8 = uni_32to8(ch32);
            rect = render(utf8, x, y, SDL2MapWindow::colors[oc], textBG(attr));
            x += rect.w;
        }
        rect = render(m_lines.lines[p].text,
                x, y,
                textFG(attr), textBG(attr));
        x += rect.w;
        if (i == 0 && m_more) {
            rect = render("--More--",
                x, y,
                textFG(ATR_INVERSE), textBG(ATR_INVERSE));
            x += rect.w;
        }
        rect.x = x;
        rect.w = width() - rect.x;
        interface()->fill(this, rect, background);
    }
    rect.x = 0;
    rect.y = 0;
    rect.w = width() + 1;
    rect.h = height() - lineHeight()*i;
    if (rect.h > 0) {
        interface()->fill(this, rect, background);
    }
}

int SDL2MessageWindow::heightHint(void)
{
    return lineHeight() * 2;
}

void SDL2MessageWindow::putMixed(int attr, const char *str, int glyph)
{
    // Add new string to m_contents
    addLine(&m_contents, attr, str, glyph);

    // Add new string to m_lines.
    // This is a new line if this is the first message for a turn, if the
    // attribute or that of the prior string is non-default, if the string is
    // one requiring emphasis, or if the width plus --More-- would exceed the
    // width of the window.
    bool add_new = false;
    bool do_more = false;
    if (!m_combine) {
        add_new = true;
    } else if (attr != ATR_NONE) {
        add_new = true;
    } else if (strcmp(str, "You die...") == 0) {
        add_new = true;
        do_more = true;
    } else if (m_lines.head == m_lines.tail) {
        add_new = true;
    } else {
        size_t tail = m_lines.tail - 1;
        if (m_lines.tail == 0) {
            tail = m_lines.size - 1;
        }
        if (m_lines.lines[tail].attributes != ATR_NONE) {
            add_new = true;
        } else {
            char str2[BUFSZ];
            int new_width;

            snprintf(str2, SIZE(str2), "%s  %s--More--",
                    m_lines.lines[tail].text, str);
            new_width = font()->textSize(str2).w;
            if (new_width > width()) {
                add_new = true;
                do_more = true;
            }
        }
    }
    // Show --More-- if we need it
    if (do_more) {
        more();
    }
    // Add the line
    if (add_new) {
        Line *new_cline = addLine(&m_lines, attr, str, glyph);
        new_cline->num_messages = 1;
    } else {
        char *buf;
        size_t tail = m_lines.tail - 1;
        if (m_lines.tail == 0) {
            tail = m_lines.size - 1;
        }
        buf = new char[strlen(m_lines.lines[tail].text) + strlen(str) + 3];
        sprintf(buf, "%s  %s", m_lines.lines[tail].text, str);
        delete[] m_lines.lines[tail].text;
        m_lines.lines[tail].text = buf;
        m_lines.lines[tail].num_messages += 1;
    }

    // Discard lines exceeding the configured count
    while (numberOfLines(&m_lines) > 1 && numberOfLines(&m_lines) > iflags.msg_history) {
        int num_messages = m_lines.lines[m_lines.head].num_messages;
        for (int i = 0; i < num_messages; ++i) {
            delete[] m_contents.lines[m_contents.head].text;
            m_contents.lines[m_contents.head].text = NULL;
            ++m_contents.head;
            if (m_contents.head >= m_contents.size) {
                m_contents.head = 0;
            }
        }
        delete[] m_lines.lines[m_lines.head].text;
        m_lines.lines[m_lines.head].text = NULL;
        ++m_lines.head;
        if (m_lines.head >= m_lines.size) {
            m_lines.head = 0;
        }
    }

    m_line_offset = 0;
    interface()->redraw();

    // Combine next string
    m_combine = true;
}

void SDL2MessageWindow::putStr(int attr, const char *str)
{
    putMixed(attr, str, NO_GLYPH);
}

void SDL2MessageWindow::putMixed(int attr, const char *str)
{
    if (str[0] == '\\' && str[1] == 'G' && strlen(str) >= 10) {
        char rndchk_str[5];
        char *end;
        strncpy(rndchk_str, str + 2, 4);
        rndchk_str[4] = '\0';
        int rndchk = strtol(rndchk_str, &end, 16);
        if (rndchk == context.rndencode && *end == '\0') {
            char gv_str[5];
            strncpy(gv_str, str + 6, 4);
            gv_str[4] = '\0';
            int gv = strtol(gv_str, &end, 16);
            if (*end == '\0') {
                putMixed(attr, str + 10, gv);
                return;
            }
        }
    }

    putMixed(attr, str, NO_GLYPH);
}

void SDL2MessageWindow::prevMessage(void)
{
    int visible = height() / lineHeight();
    if (visible < numberOfLines(&m_lines)) {
        int max_scroll = numberOfLines(&m_lines) - visible;
        if (m_line_offset < max_scroll) {
            ++m_line_offset;
            interface()->redraw();
        }
    }
}

void SDL2MessageWindow::more(void)
{
    m_more = true;
    interface()->getKey();
    m_more = false;
}

void SDL2MessageWindow::initLineList(LineList *list, size_t size)
{
    size_t i;

    list->size = size + 1;
    list->lines = new Line[list->size];
    list->head = 0;
    list->tail = 0;
    for (i = 0; i < list->size; ++i) {
        list->lines[i].text = NULL;
    }
}

void SDL2MessageWindow::freeLineList(LineList *list)
{
    size_t i;

    for (i = 0; i < list->size; ++i) {
        delete[] list->lines[i].text;
    }
    delete[] list->lines;
}

size_t SDL2MessageWindow::numberOfLines(const LineList *list)
{
    size_t count = list->tail - list->head;
    if (list->tail < list->head) {
        count += list->size;
    }
    return count;
}

SDL2MessageWindow::Line *SDL2MessageWindow::addLine(
        LineList *list, int attr, const char *str, int glyph)
{
    Line *new_line = &list->lines[list->tail++];
    if (list->tail >= list->size) {
        list->tail = 0;
    }
    if (list->tail == list->head) {
        ++list->head;
        if (list->head >= list->size) {
            list->head = 0;
        }
    }
    delete[] new_line->text;
    new_line->text = new char[strlen(str) + 1];
    strcpy(new_line->text, str);
    new_line->attributes = attr;
    new_line->glyph = glyph;
    return new_line;
}

}
