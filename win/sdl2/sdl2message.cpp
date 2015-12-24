// sdl2message.cpp

extern "C" {
#include "hack.h"
}
#include "sdl2.h"
#include "sdl2message.h"
#include "sdl2interface.h"
#include "sdl2font.h"

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
}

void SDL2MessageWindow::redraw(void)
{
    const SDL_Color background = {   0,   0,   0, 255 };
    int num_lines;  // Number of visible lines
    int i;
    SDL_Rect rect;

    num_lines = (height() + lineHeight() - 1) / lineHeight();
    std::list<CombinedLine>::reverse_iterator p = m_lines.rbegin();
    for (i = 0; i < m_line_offset && p != m_lines.rend(); ++i) {
        ++p;
    }
    for (i = 0; i < num_lines && p != m_lines.rend(); ++i) {
        int attr = p->attributes;
        int x = 0;
        rect = render(p->text,
                x, height() - lineHeight()*(i+1),
                textFG(attr), textBG(attr));
        if (i == 0 && m_more) {
            x += rect.w;
            rect = render("--More--",
                x, height() - lineHeight()*(i+1),
                textFG(ATR_INVERSE), textBG(ATR_INVERSE));
        }
        rect.x += rect.w;
        rect.w = width() - rect.x;
        interface()->fill(this, rect, background);
        ++p;
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

void SDL2MessageWindow::putStr(int attr, const std::string& str)
{
    // Add new string to m_contents
    Line new_line;
    new_line.text = str;
    new_line.attributes = attr;
    m_contents.push_back(new_line);

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
    } else if (str == "You die...") {
        add_new = true;
        do_more = true;
    } else if (m_lines.empty()) {
        add_new = true;
    } else if (m_lines.rbegin()->attributes != ATR_NONE) {
        add_new = true;
    } else {
        int new_width = (font()->textSize(m_lines.rbegin()->text + "  " + str
                + "--More--").w);
        if (new_width > width()) {
            add_new = true;
            do_more = true;
        }
    }
    // Show --More-- if we need it
    if (do_more) {
        more();
    }
    // Add the line
    if (add_new) {
        CombinedLine new_cline;

        new_cline.text = str;
        new_cline.attributes = attr;
        new_cline.num_messages = 1;
        m_lines.push_back(new_cline);
    } else {
        m_lines.rbegin()->text += "  " + str;
        m_lines.rbegin()->num_messages += 1;
    }

    // Discard lines exceeding the configured count
    while (m_lines.size() > 1 && m_lines.size() > iflags.msg_history) {
        int num_messages = m_lines.begin()->num_messages;
        for (int i = 0; i < num_messages; ++i) {
            m_contents.pop_front();
        }
        m_lines.pop_front();
    }

    m_line_offset = 0;
    interface()->redraw();

    // Combine next string
    m_combine = true;
}

void SDL2MessageWindow::prevMessage(void)
{
    int visible = height() / lineHeight();
    if (visible < m_contents.size()) {
        int max_scroll = m_contents.size() - visible;
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

}
