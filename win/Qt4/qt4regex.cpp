extern "C" {
#include "hack.h"
}

#include <QtCore/QRegExp>

struct nhregex {
    QRegExp *re;

    nhregex(void) : re(NULL) {}
    ~nhregex(void) { delete re; }
};

extern "C" {

extern const char regex_id[] = "qtregex";

struct nhregex *
regex_init(void)
{
    return new nhregex;
}

boolean
regex_compile(const char *s, struct nhregex *re)
{
    if (!re) {
        return FALSE;
    }
    re->re = new QRegExp(s);
    return re->re->isValid();
}

const char *
regex_error_desc(struct nhregex *re)
{
    if (re->re == NULL) {
        return "No regular expression";
    }
    if (re->re->isValid()) {
        return NULL;
    }
    return re->re->errorString().toUtf8().constData();
}

boolean
regex_match(const char *s, struct nhregex *re)
{
    if (re->re == NULL) {
        return FALSE;
    }
    return re->re->indexIn(s) >= 0;
}

void
regex_free(struct nhregex *re)
{
    delete re;
}

}
