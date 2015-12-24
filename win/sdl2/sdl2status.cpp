// sdl2status.cpp

extern "C" {
#include "hack.h"
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
    // Avoid segfaults if certain data structures are not ready
    if (youmonst.data == NULL) return;

    // TODO:  may need adjustment for right-to-left scripts
    // TODO:  implement status colors here
    static const SDL_Color black = { 0, 0, 0, 255 };
    int x, y;
    std::string str;
    char str2[BUFSZ];
    int cap, hp, hpmax;
    SDL_Rect rect;

    interface()->fill(this, black);

    // First line:
    x = 0;
    y = 0;
    // Name and rank or monster
    {
        std::string rankstr;

        if (Upolyd)
            rankstr = mons[u.umonnum].mname;
        else
            rankstr = rank_of(u.ulevel, Role_switch, flags.female);
        snprintf(str2, SIZE(str2), "%s the %s", plname, rankstr.c_str());
    }
    rect = render(str2, x, y, textColor(sc->lev_color()), textBG(ATR_NONE));
    x += rect.w;

    // Format the strength score as nn or 18/nn
    if (ACURR(A_STR) > 18) {
        if (ACURR(A_STR) > STR18(100))
            snprintf(str2, SIZE(str2), " St:%d", ACURR(A_STR)-100);
        else if (ACURR(A_STR) < STR18(100))
            snprintf(str2, SIZE(str2), " St:18/%02d", ACURR(A_STR)-18);
        else
            snprintf(str2, SIZE(str2), " St:18/**");
    } else {
        snprintf(str2, SIZE(str2), " St:%d", ACURR(A_STR));
    }
    rect = render(str2, x, y, textColor(sc->str_color()), textBG(ATR_NONE));
    x += rect.w;

    // Other attributes
    snprintf(str2, SIZE(str2), " Dx:%d", ACURR(A_DEX));
    rect = render(str2, x, y, textColor(sc->dex_color()), textBG(ATR_NONE));
    x += rect.w;

    snprintf(str2, SIZE(str2), " Co:%d", ACURR(A_CON));
    rect = render(str2, x, y, textColor(sc->con_color()), textBG(ATR_NONE));
    x += rect.w;

    snprintf(str2, SIZE(str2), " In:%d", ACURR(A_INT));
    rect = render(str2, x, y, textColor(sc->int_color()), textBG(ATR_NONE));
    x += rect.w;

    snprintf(str2, SIZE(str2), " Wi:%d", ACURR(A_WIS));
    rect = render(str2, x, y, textColor(sc->wis_color()), textBG(ATR_NONE));
    x += rect.w;

    snprintf(str2, SIZE(str2), " Ch:%d", ACURR(A_CHA));
    rect = render(str2, x, y, textColor(sc->cha_color()), textBG(ATR_NONE));
    x += rect.w;

    // Alignment
    snprintf(str2, SIZE(str2), "  %s",
            (u.ualign.type == A_CHAOTIC) ? "Chaotic" :
            (u.ualign.type == A_NEUTRAL) ? "Neutral" : "Lawful");
    rect = render(str2, x, y, textColor(sc->align_color()), textBG(ATR_NONE));
    x += rect.w;

#ifdef SCORE_ON_BOTL
    // Score
    if (flags.showscore)
    {
        snprintf(str2, SIZE(str2), "  S:%1$d", botl_score());
        rect = render(str2, x, y, textFG(ATR_NONE), textBG(ATR_NONE));
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
    describe_level(str2);
    rect = render(str2, x, y, textFG(ATR_NONE), textBG(ATR_NONE));
    x += rect.w;

    // Gold
    snprintf(str2, SIZE(str2), " %s:%-2ld",
            encglyph(objnum_to_glyph(GOLD_PIECE)), money_cnt(invent));
    rect = render(str2, x, y, textColor(sc->gold_color()), textBG(ATR_NONE));
    x += rect.w;

    // Current hit points
    snprintf(str2, SIZE(str2), " HP:%1$d", hp);
    rect = render(str2, x, y, textColor(sc->hp_color()), textBG(ATR_NONE));
    x += rect.w;
    // Maximum hit points
    snprintf(str2, SIZE(str2), "(%d)", hpmax);
    rect = render(str2, x, y, textColor(sc->hpmax_color()), textBG(ATR_NONE));
    x += rect.w;

    // Current energy points
    snprintf(str2, SIZE(str2), " Pw:%d", u.uen);
    rect = render(str2, x, y, textColor(sc->en_color()), textBG(ATR_NONE));
    x += rect.w;
    // Maximum energy points
    snprintf(str2, SIZE(str2), "(%d)", u.uenmax);
    rect = render(str2, x, y, textColor(sc->enmax_color()), textBG(ATR_NONE));
    x += rect.w;

    // Armor class
    snprintf(str2, SIZE(str2), " AC:%-2d", u.uac);
    rect = render(str2, x, y, textColor(sc->ac_color()), textBG(ATR_NONE));
    x += rect.w;

    // Experience
    if (Upolyd)
        snprintf(str2, SIZE(str2), " HD:%d", mons[u.umonnum].mlevel);
#ifdef EXP_ON_BOTL
    else if (flags.showexp)
        snprintf(str2, SIZE(str2), " Xp:%u/%-1d", u.ulevel, u.uexp);
#endif
    else
        snprintf(str2, SIZE(str2), " Exp:%u", u.ulevel);
    rect = render(str2, x, y, textColor(sc->lev_color()), textBG(ATR_NONE));
    x += rect.w;

    // Time
    if (flags.time) {
        snprintf(str2, SIZE(str2), " T:%ld", moves);
        rect = render(str2, x, y, textFG(ATR_NONE), textBG(ATR_NONE));
        x += rect.w;
    }

    // Hunger
    if (strcmp(hu_stat[u.uhs], "        ") != 0) {
        str = std::string(" ") + hu_stat[u.uhs];
        rect = render(str, x, y, textColor(sc->hunger_color()), textBG(ATR_NONE));
        x += rect.w;
    }

    // Flags
    str = "";
    if (Confusion)
        str += " Conf";
    if (u.usick_type & SICK_VOMITABLE)
        str += " FoodPois";
    if (u.usick_type & SICK_NONVOMITABLE)
        str += " Ill";
    if (Blind)
        str += " Blind";
    if (Stunned)
        str += " Stun";
    if (Hallucination)
        str += " Hallu";
    if (Slimed)
        str += " Slime";
    rect = render(str, x, y, textFG(ATR_NONE), textBG(ATR_NONE));
    x += rect.w;

    // Encumbrance
    if (cap > UNENCUMBERED) {
        str = std::string(" ") + enc_stat[cap];
        rect = render(str, x, y, textColor(sc->encumb_color()), textBG(ATR_NONE));
        x += rect.w;
    }
}

int SDL2StatusWindow::heightHint(void)
{
    return lineHeight() * 2;
}

void SDL2StatusWindow::putStr(int /*attr*/, const std::string& /*str*/)
{
}

void SDL2StatusWindow::clear(void)
{
}

void SDL2StatusWindow::setCursor(int /*x*/, int /*y*/)
{
}

}
