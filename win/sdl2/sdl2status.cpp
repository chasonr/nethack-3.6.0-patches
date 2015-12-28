// sdl2status.cpp

extern "C" {
#include "hack.h"
#include "unicode.h"
}
#include "sdl2.h"
#include "sdl2status.h"
#include "sdl2font.h"
#include "sdl2interface.h"

extern const char *hu_stat[];         /* defined in eat.cpp */
extern const char * const enc_stat[]; /* defined in botl.cpp */

// TODO: games with the preprocessor until status colors done
#define textColor(x) textFG(ATR_NONE)

namespace NH_SDL2
{

SDL2StatusWindow::SDL2StatusWindow(SDL2Interface *interface) :
    SDL2Window(interface)
{
    // Status window font
    setFont(iflags.wc_font_status, iflags.wc_fontsiz_status,
            SDL2Font::defaultSerifFont(), 20);
}

void SDL2StatusWindow::redraw(void)
{
    StringContext ctx("SDL2StatusWindow::redraw");

    // Avoid segfaults if certain data structures are not ready
    if (youmonst.data == NULL) return;

    // TODO:  may need adjustment for right-to-left scripts
    // TODO:  implement status colors here
    static const SDL_Color black = { 0, 0, 0, 255 };
    int x, y;
    char str[BUFSZ];
    int cap, hp, hpmax;
    SDL_Rect rect;

    interface()->fill(this, black);

    // First line:
    x = 0;
    y = 0;
    // Name and rank or monster
    {
        const char *rankstr;

        if (Upolyd)
            rankstr = mons[u.umonnum].mname;
        else
            rankstr = rank_of(u.ulevel, Role_switch, flags.female);
        snprintf(str, SIZE(str), "%s the %s", plname, rankstr);
    }
    rect = render(str, x, y, textColor(sc->lev_color()), textBG(ATR_NONE));
    x += rect.w;

    // Format the strength score as nn or 18/nn
    if (ACURR(A_STR) > 18) {
        if (ACURR(A_STR) > STR18(100))
            snprintf(str, SIZE(str), " St:%d", ACURR(A_STR)-100);
        else if (ACURR(A_STR) < STR18(100))
            snprintf(str, SIZE(str), " St:18/%02d", ACURR(A_STR)-18);
        else
            snprintf(str, SIZE(str), " St:18/**");
    } else {
        snprintf(str, SIZE(str), " St:%d", ACURR(A_STR));
    }
    rect = render(str, x, y, textColor(sc->str_color()), textBG(ATR_NONE));
    x += rect.w;

    // Other attributes
    snprintf(str, SIZE(str), " Dx:%d", ACURR(A_DEX));
    rect = render(str, x, y, textColor(sc->dex_color()), textBG(ATR_NONE));
    x += rect.w;

    snprintf(str, SIZE(str), " Co:%d", ACURR(A_CON));
    rect = render(str, x, y, textColor(sc->con_color()), textBG(ATR_NONE));
    x += rect.w;

    snprintf(str, SIZE(str), " In:%d", ACURR(A_INT));
    rect = render(str, x, y, textColor(sc->int_color()), textBG(ATR_NONE));
    x += rect.w;

    snprintf(str, SIZE(str), " Wi:%d", ACURR(A_WIS));
    rect = render(str, x, y, textColor(sc->wis_color()), textBG(ATR_NONE));
    x += rect.w;

    snprintf(str, SIZE(str), " Ch:%d", ACURR(A_CHA));
    rect = render(str, x, y, textColor(sc->cha_color()), textBG(ATR_NONE));
    x += rect.w;

    // Alignment
    snprintf(str, SIZE(str), "  %s",
            (u.ualign.type == A_CHAOTIC) ? "Chaotic" :
            (u.ualign.type == A_NEUTRAL) ? "Neutral" : "Lawful");
    rect = render(str, x, y, textColor(sc->align_color()), textBG(ATR_NONE));
    x += rect.w;

#ifdef SCORE_ON_BOTL
    // Score
    if (flags.showscore)
    {
        snprintf(str, SIZE(str), "  S:%1$d", botl_score());
        rect = render(str, x, y, textFG(ATR_NONE), textBG(ATR_NONE));
        x += rect.w;
    }
#endif

    // Second line:
    x = 0;
    y += lineHeight();

    cap = near_capacity();

    hp = Upolyd ? u.mh : u.uhp;
    hpmax = Upolyd ? u.mhmax : u.uhpmax;

    // Current level
    if (hp < 0) hp = 0;
    describe_level(str);
    rect = render(str, x, y, textFG(ATR_NONE), textBG(ATR_NONE));
    x += rect.w;

    // Gold
    {
        utf32_t ch32[2];
        ch32[0] = chrConvert(showsyms[COIN_CLASS + SYM_OFF_O]);
        ch32[1] = 0;
        snprintf(str, SIZE(str), " %s:%-2ld",
                uni_32to8(ch32), money_cnt(invent));
        rect = render(str, x, y, textColor(sc->gold_color()), textBG(ATR_NONE));
    }
    x += rect.w;

    // Current hit points
    snprintf(str, SIZE(str), " HP:%1$d", hp);
    rect = render(str, x, y, textColor(sc->hp_color()), textBG(ATR_NONE));
    x += rect.w;
    // Maximum hit points
    snprintf(str, SIZE(str), "(%d)", hpmax);
    rect = render(str, x, y, textColor(sc->hpmax_color()), textBG(ATR_NONE));
    x += rect.w;

    // Current energy points
    snprintf(str, SIZE(str), " Pw:%d", u.uen);
    rect = render(str, x, y, textColor(sc->en_color()), textBG(ATR_NONE));
    x += rect.w;
    // Maximum energy points
    snprintf(str, SIZE(str), "(%d)", u.uenmax);
    rect = render(str, x, y, textColor(sc->enmax_color()), textBG(ATR_NONE));
    x += rect.w;

    // Armor class
    snprintf(str, SIZE(str), " AC:%-2d", u.uac);
    rect = render(str, x, y, textColor(sc->ac_color()), textBG(ATR_NONE));
    x += rect.w;

    // Experience
    if (Upolyd)
        snprintf(str, SIZE(str), " HD:%d", mons[u.umonnum].mlevel);
    else if (flags.showexp)
        snprintf(str, SIZE(str), " Xp:%u/%-1ld", u.ulevel, u.uexp);
    else
        snprintf(str, SIZE(str), " Exp:%u", u.ulevel);
    rect = render(str, x, y, textColor(sc->lev_color()), textBG(ATR_NONE));
    x += rect.w;

    // Time
    if (flags.time) {
        snprintf(str, SIZE(str), " T:%ld", moves);
        rect = render(str, x, y, textFG(ATR_NONE), textBG(ATR_NONE));
        x += rect.w;
    }

    // Hunger
    if (strcmp(hu_stat[u.uhs], "        ") != 0) {
        snprintf(str, SIZE(str), " %s", hu_stat[u.uhs]);
        rect = render(str, x, y, textColor(sc->hunger_color()), textBG(ATR_NONE));
        x += rect.w;
    }

    // Flags
    snprintf(str, SIZE(str), "%s%s%s%s%s%s%s",
            (Confusion) ? " Conf" : "",
            (u.usick_type & SICK_VOMITABLE) ? " FoodPois" : "",
            (u.usick_type & SICK_NONVOMITABLE) ?  " Ill" : "",
            (Blind) ? " Blind" : "",
            (Stunned) ? " Stun" : "",
            (Hallucination) ? " Hallu" : "",
            (Slimed) ? " Slime" : "");
    rect = render(str, x, y, textFG(ATR_NONE), textBG(ATR_NONE));
    x += rect.w;

    // Encumbrance
    if (cap > UNENCUMBERED) {
        snprintf(str, SIZE(str), " %s", enc_stat[cap]);
        rect = render(str, x, y, textColor(sc->encumb_color()), textBG(ATR_NONE));
        x += rect.w;
    }
}

int SDL2StatusWindow::heightHint(void)
{
    return lineHeight() * 2;
}

void SDL2StatusWindow::putStr(int /*attr*/, const char * /*str*/)
{
}

void SDL2StatusWindow::clear(void)
{
}

void SDL2StatusWindow::setCursor(int /*x*/, int /*y*/)
{
}

}
