// sdlplsel.cpp

extern "C" {
#include "hack.h"
}
#include "unicode.h"
#include "sdl2.h"
#include "sdl2plsel.h"
#include "sdl2menu.h"

namespace NH_SDL2
{

static bool selectRole(SDL2Interface *interface);
static bool selectRace(SDL2Interface *interface);
static bool selectGender(SDL2Interface *interface);
static bool selectAlignment(SDL2Interface *interface);

static bool validSelection(int role, int race, int gender, int align);

bool SDL2PlayerSelect(SDL2Interface *interface)
{
    return true
        && selectRole(interface)
        && selectRace(interface)
        && selectGender(interface)
        && selectAlignment(interface);
}

static bool selectRole(SDL2Interface *interface)
{
    if (flags.initrole != -1) {
        return true;
    }

    int role = -1;
    unsigned count = 0;
    anything id;

    SDL2Menu menu(interface);

    menu.startMenu();

    id.a_void = NULL;
    for (int i = 0; roles[i].name.m != NULL; ++i) {
        if (validSelection(i, flags.initrace, flags.initgend, flags.initalign)) {
            const char *name_m = roles[i].name.m;
            const char *name_f = roles[i].name.f;
            char text[BUFSZ];

            if (flags.initgend == 0 || name_f == NULL) {
                snprintf(text, SIZE(text), "%s", name_m);
            } else if (flags.initgend == 1) {
                snprintf(text, SIZE(text), "%s", name_f);
            } else {
                snprintf(text, SIZE(text), "%s/%s", name_m, name_f);
            }

            role = i;
            ++count;
            id.a_int = i + 1;
            menu.addMenu(NO_GLYPH, &id, 0, 0, 0, text, false);
        }
    }

    // Bail if no valid roles
    if (count == 0) {
        return false;
    }

    // If only one role is compatible with other settings, don't bother with
    // the menu
    if (count == 1) {
        flags.initrole = role;
        return true;
    }

    id.a_int = -1;
    menu.addMenu(NO_GLYPH, &id, '*', 0, 0, "Random", false);

    menu.endMenu("Choose your role:");

    menu_item *selection;
    int rc = menu.selectMenu(PICK_ONE, &selection);
    if (rc <= 0) {
        return false;
    }

    role = selection[0].item.a_int - 1;
    if (role < 0) {
        role = pick_role(flags.initrace, flags.initgend, flags.initalign, PICK_RANDOM);
    }

    flags.initrole = role;
    return true;
}

static bool selectRace(SDL2Interface *interface)
{
    if (flags.initrace != -1) {
        return true;
    }

    int race = -1;
    unsigned count = 0;
    anything id;

    SDL2Menu menu(interface);

    menu.startMenu();

    id.a_void = 0;
    for (int i = 0; races[i].noun != NULL; ++i) {
        if (validSelection(flags.initrole, i, flags.initgend, flags.initalign)) {
            std::string text = races[i].noun;

            race = i;
            ++count;
            id.a_int = i + 1;
            menu.addMenu(NO_GLYPH, &id, 0, 0, 0, text, false);
        }
    }

    // Bail if no valid races
    if (count == 0) {
        return false;
    }

    // If only one race is compatible with other settings, don't bother with
    // the menu
    if (count == 1) {
        flags.initrace = race;
        return true;
    }

    id.a_int = -1;
    menu.addMenu(NO_GLYPH, &id, '*', 0, 0, "Random", false);

    menu.endMenu("Choose your race:");

    menu_item *selection;
    int rc = menu.selectMenu(PICK_ONE, &selection);
    if (rc <= 0) {
        return false;
    }

    race = selection[0].item.a_int - 1;
    if (race < 0) {
        race = pick_race(flags.initrole, flags.initgend, flags.initalign, PICK_RANDOM);
    }

    flags.initrace = race;
    return true;
}

static bool selectGender(SDL2Interface *interface)
{
    if (flags.initgend != -1) {
        return true;
    }

    int gend = -1;
    unsigned count = 0;
    anything id;

    SDL2Menu menu(interface);

    menu.startMenu();

    id.a_void = NULL;
    for (int i = 0; genders[i].adj != NULL; ++i) {
        if (validSelection(flags.initrole, flags.initrace, i, flags.initalign)) {
            std::string text = genders[i].adj;

            gend = i;
            ++count;
            id.a_int = i + 1;
            menu.addMenu(NO_GLYPH, &id, 0, 0, 0, text, false);
        }
    }

    // Bail if no valid genders
    if (count == 0) {
        return false;
    }

    // If only one gender is compatible with other settings, don't bother with
    // the menu
    if (count == 1) {
        flags.initgend = gend;
        return true;
    }

    id.a_int = -1;
    menu.addMenu(NO_GLYPH, &id, '*', 0, 0, "Random", false);

    menu.endMenu("Choose your gender:");

    menu_item *selection;
    int rc = menu.selectMenu(PICK_ONE, &selection);
    if (rc <= 0) {
        return false;
    }

    gend = selection[0].item.a_int - 1;
    if (gend < 0) {
        gend = pick_gend(flags.initrole, flags.initrace, flags.initalign, PICK_RANDOM);
    }

    flags.initgend = gend;
    return true;
}

static bool selectAlignment(SDL2Interface *interface)
{
    if (flags.initalign != -1) {
        return true;
    }

    int align = -1;
    unsigned count = 0;
    anything id;

    SDL2Menu menu(interface);

    menu.startMenu();

    id.a_void = NULL;
    for (int i = 0; aligns[i].adj != NULL; ++i) {
        if (validSelection(flags.initrole, flags.initrace, flags.initgend, i)) {
            std::string text = aligns[i].adj;

            align = i;
            ++count;
            id.a_int = i + 1;
            menu.addMenu(NO_GLYPH, &id, 0, 0, 0, text, false);
        }
    }

    // Bail if no valid alignments
    if (count == 0) {
        return false;
    }

    // If only one alignment is compatible with other settings, don't bother with
    // the menu
    if (count == 1) {
        flags.initalign = align;
        return true;
    }

    id.a_int = -1;
    menu.addMenu(NO_GLYPH, &id, '*', 0, 0, "Random", false);

    menu.endMenu("Choose your alignment:");

    menu_item *selection;
    int rc = menu.selectMenu(PICK_ONE, &selection);
    if (rc <= 0) {
        return false;
    }

    align = selection[0].item.a_int - 1;
    if (align < 0) {
        align = pick_align(flags.initrole, flags.initrace, flags.initgend, PICK_RANDOM);
    }

    flags.initalign = align;
    return true;
}

static bool validSelection(int role, int race, int gender, int align)
{
    // Any input set to -1 is unspecified; the combination is valid if there
    // exists a setting which is valid with the others
    if (role == -1) {
        for (role = 0; roles[role].name.m != NULL; ++role) {
            if (validSelection(role, race, gender, align)) {
                return true;
            }
        }
        return false;
    }

    if (race == -1) {
        for (race = 0; races[race].noun != NULL; ++race) {
            if (validSelection(role, race, gender, align)) {
                return true;
            }
        }
        return false;
    }

    if (gender == -1) {
        for (gender = 0; gender < ROLE_GENDERS; ++gender) {
            if (validSelection(role, race, gender, align)) {
                return true;
            }
        }
        return false;
    }

    if (align == -1) {
        for (align = 0; align < ROLE_ALIGNS; ++align) {
            if (validSelection(role, race, gender, align)) {
                return true;
            }
        }
        return false;
    }

    return validrole(role)
        && validrace(role, race)
        && validgend(role, race, gender)
        && validalign(role, race, align);
}

}
