/* unicode.c -- common transformations on Unicode strings */

#include "hack.h"
#include "unicode.h"

/* Structures for the Unicode database: */

/* Composition; used only for this as it is the only property of pairs of
   code points rather than a single character */
typedef struct comp_entry
{
    utf32_t ch1, ch2;
    utf32_t comp;
} comp_entry;

static utf32_t FDECL(comp_search, (
        const comp_entry *table, size_t tblen,
        utf32_t ch1, utf32_t ch2));

/* Ranges of code points having a common integer property */
typedef struct int_range_entry
{
    utf32_t wch1, wch2;
    int value;
} int_range_entry;

static const int_range_entry *FDECL(int_range_search, (
        const int_range_entry *table, size_t tblen, utf32_t wch));

/* Code points mapping to a one or more code points; char_entry lists
   singleton mappings and string_entry lists one-to-many mappings */
typedef struct char_entry
{
    utf32_t wch;
    utf32_t value[2];
} char_entry;

typedef struct string_entry
{
    utf32_t wch;
    const utf32_t *string;
} string_entry;

static const utf32_t *FDECL(string_search, (
        const char_entry *table_1, size_t tb1len,
        const string_entry *table_m, size_t tbmlen,
        utf32_t wch));

/* Ranges of code points mapping to a common 8-bit string property */
typedef struct string8_range_entry
{
    utf32_t wch1, wch2;
    const char *string;
} string8_range_entry;

static const char *FDECL(string8_range_search, (
        const string8_range_entry *table, size_t tblen, utf32_t wch));

/* Ranges of code points for which some property is true */
typedef struct boolean_entry
{
    utf32_t wch1, wch2;
} boolean_entry;

static int FDECL(boolean_search, (
        const boolean_entry *table, size_t tblen, utf32_t wch));

#include "unitbl.h"

static size_t FDECL(utf8_convert_length, (const char *, BOOLEAN_P));

size_t
uni_decomposition(out, out_size, wch, compat)
utf32_t *out;
size_t out_size;
utf32_t wch;
boolean compat;
{
    utf32_t dc[4];
    const utf32_t *dc0;
    size_t i;
    size_t retsize;

    /* Hangul */
    if (0xAC00 <= wch && wch <= 0xD7A3) {
        unsigned a, b, c;

        a = (unsigned)(wch - 0xAC00);
        b = a / 28;
        a %= 28;
        c = b / 21;
        b %= 21;
        dc[0] = c + 0x1100;
        dc[1] = b + 0x1161;
        if (a == 0) {
            dc[2] = 0;
        } else {
            dc[2] = a + 0x11A7;
            dc[3] = 0;
        }
        return uni_copy32(out, out_size, dc);
    }

    /* Other decomposable characters */
    /* Reject compatibility decompositions if canonical decompositions
       requested */
    if (!compat
    &&  string8_range_search(dctype_table, SIZE(dctype_table), wch) != NULL) {
        goto no_decomposition;
    }

    /* Search for the decomposition */
    dc0 = string_search(
            decomp1_table, SIZE(decomp1_table),
            decomp_table, SIZE(decomp_table), wch);
    if (dc0 == NULL) {
        goto no_decomposition; /* No decomposition; don't go into recursion */
    }

    /* Decompose recursively */
    retsize = 0;
    for (i = 0; dc0[i] != 0; ++i) {
        size_t j = uni_decomposition(out, out_size, dc0[i], compat);
        retsize += j;
        if (out != NULL && out_size != 0) {
            out += j;
            if (out_size >= j) {
                out_size -= j;
            } else {
                out_size = 0;
            }
        }
    }

    return retsize;

no_decomposition:
    if (out != NULL && out_size > 0) {
        if (out_size == 1) {
            out[0] = 0;
        } else {
            out[0] = wch;
            out[1] = 0;
        }
    }
    return 1;
}

utf32_t
uni_composition(wch1, wch2)
utf32_t wch1;
utf32_t wch2;
{
    /* Hangul open syllables */
    if (0x1100 <= wch1 && wch1 <= 0x1112
    &&  0x1161 <= wch2 && wch2 <= 0x1175)
        return 0xAC00 + (wch1-0x1100)*588 + (wch2-0x1161)*28;
    /* Hangul closed syllables */
    if (0xAC00 <= wch1 && wch1 <= 0xD7A3 && (wch1-0xAC00)%28 == 0
    &&  0x11A8 <= wch2 && wch2 <= 0x11C2)
        return wch1 + wch2 - 0x11A7;

    /* Other compositions */
    return comp_search(comp_table, SIZE(comp_table), wch1, wch2);
}

int
uni_combining_class(wch)
utf32_t wch;
{
    const int_range_entry *elem;

    elem = int_range_search(comb_class_table, SIZE(comb_class_table), wch);
    if (elem == NULL) {
        return 0;
    }
    return elem->value;
}

/* Output length for converting UTF-8 */
static size_t
utf8_convert_length(inp, utf16)
const char *inp;
boolean utf16;
{
    size_t i, outlen;

    outlen = 0;
    i = 0;
    while (inp[i] != 0) {
        unsigned char byte = inp[i++];
        utf32_t ch32;
        utf32_t min = 0;
        unsigned count = 0;

        if (byte < 0x80) {
            ch32 = byte;
        } else if (byte < 0xC0) {
            ch32 = 0xFFFD;
        } else if (byte < 0xE0) {
            ch32 = byte & 0x1F;
            min = 0x80;
            count = 1;
        } else if (byte < 0xF0) {
            ch32 = byte & 0x0F;
            min = 0x800;
            count = 2;
        } else if (byte < 0xF5) {
            ch32 = byte & 0x07;
            min = 0x10000;
            count = 3;
        } else {
            ch32 = 0xFFFD;
        }

        for (; count != 0; --count) {
            byte = inp[i];
            if ((byte & 0xC0) != 0x80) {
                break;
            }
            ++i;
            ch32 = (ch32 << 6) | (byte & 0x3F);
        }
        if (count != 0 || ch32 < min || ((ch32 & 0xFFFFF800) == 0xD800)) {
            ch32 = 0xFFFD;
        }
        ++outlen;
        if (utf16 && ch32 >= 0x10000) {
            ++outlen;
        }
    }

    return outlen;
}

utf16_t *
uni_8to16(inp)
const char *inp;
{
    size_t i, j;
    utf16_t *out;

    /* Output string */
    out = str_alloc16(utf8_convert_length(inp, TRUE));

    i = 0;
    j = 0;
    while (inp[i] != 0) {
        unsigned char byte = inp[i++];
        utf32_t ch32;
        utf32_t min = 0;
        unsigned count = 0;

        if (byte < 0x80) {
            ch32 = byte;
        } else if (byte < 0xC0) {
            ch32 = 0xFFFD;
        } else if (byte < 0xE0) {
            ch32 = byte & 0x1F;
            min = 0x80;
            count = 1;
        } else if (byte < 0xF0) {
            ch32 = byte & 0x0F;
            min = 0x800;
            count = 2;
        } else if (byte < 0xF5) {
            ch32 = byte & 0x07;
            min = 0x10000;
            count = 3;
        } else {
            ch32 = 0xFFFD;
        }

        for (; count != 0; --count) {
            byte = inp[i];
            if ((byte & 0xC0) != 0x80) {
                break;
            }
            ++i;
            ch32 = (ch32 << 6) | (byte & 0x3F);
        }
        if (count != 0 || ch32 < min || ((ch32 & 0xFFFFF800) == 0xD800)) {
            ch32 = 0xFFFD;
        }
        if (ch32 >= 0x10000) {
            out[j++] = 0xD7C0 + (ch32 >> 10);
            out[j++] = 0xDC00 + (ch32 & 0x3FF);
        } else {
            out[j++] = ch32;
        }
    }

    out[j] = 0;
    return out;
}

utf32_t *
uni_8to32(inp)
const char *inp;
{
    size_t i, j;
    utf32_t *out;

    /* Output string */
    out = str_alloc32(utf8_convert_length(inp, FALSE));

    i = 0;
    j = 0;
    while (inp[i] != 0) {
        unsigned char byte = inp[i++];
        utf32_t ch32;
        utf32_t min = 0;
        unsigned count = 0;

        if (byte < 0x80) {
            ch32 = byte;
        } else if (byte < 0xC0) {
            ch32 = 0xFFFD;
        } else if (byte < 0xE0) {
            ch32 = byte & 0x1F;
            min = 0x80;
            count = 1;
        } else if (byte < 0xF0) {
            ch32 = byte & 0x0F;
            min = 0x800;
            count = 2;
        } else if (byte < 0xF5) {
            ch32 = byte & 0x07;
            min = 0x10000;
            count = 3;
        } else {
            ch32 = 0xFFFD;
        }

        for (; count != 0; --count) {
            byte = inp[i];
            if ((byte & 0xC0) != 0x80) {
                break;
            }
            ++i;
            ch32 = (ch32 << 6) | (byte & 0x3F);
        }
        if (count != 0 || ch32 < min || ((ch32 & 0xFFFFF800) == 0xD800)) {
            ch32 = 0xFFFD;
        }
        out[j++] = ch32;
    }

    out[j] = 0;
    return out;
}

char *
uni_16to8(inp)
const utf16_t *inp;
{
    size_t i, j;
    char *out;
    size_t outsize;

    /* Size of output string */
    outsize = 0;
    i = 0;
    while (inp[i] != 0) {
        utf32_t ch32 = inp[i++];
        if ((ch32 & 0xFC00) == 0xD800
        &&  (inp[i] & 0xFC00) == 0xDC00) {
            outsize += 4;
            ++i;
        } else if (ch32 < 0x80) {
            outsize += 1;
        } else if (ch32 < 0x800) {
            outsize += 2;
        } else {
            outsize += 3;
        } 
    }

    out = str_alloc(outsize);

    i = 0;
    j = 0;
    while (inp[i] != 0) {
        utf32_t ch32 = inp[i++];

        if ((ch32 & 0xFC00) == 0xD800
        &&  (inp[i] & 0xFC00) == 0xDC00) {
            ch32 = 0x10000 + ((ch32 & 0x3FF) << 10) + (inp[i++] & 0x3FF);
        } else if ((ch32 & 0xF800) == 0xD800) {
            ch32 = 0xFFFD;
        }

        if (ch32 < 0x80) {
            out[j++] = (char) ch32;
        } else if (ch32 < 0x800) {
            out[j++] = (char) (0xC0 |  (ch32 >>  6)        );
            out[j++] = (char) (0x80 | ( ch32        & 0x3F));
        } else if (ch32 < 0x10000) {
            out[j++] = (char) (0xE0 |  (ch32 >> 12)        );
            out[j++] = (char) (0x80 | ((ch32 >>  6) & 0x3F));
            out[j++] = (char) (0x80 | ( ch32        & 0x3F));
        } else {
            out[j++] = (char) (0xF0 |  (ch32 >> 18)        );
            out[j++] = (char) (0x80 | ((ch32 >> 12) & 0x3F));
            out[j++] = (char) (0x80 | ((ch32 >>  6) & 0x3F));
            out[j++] = (char) (0x80 | ( ch32        & 0x3F));
        }
    }

    out[j] = '\0';
    return out;
}

utf32_t *
uni_16to32(inp)
const utf16_t *inp;
{
    size_t i, j;
    utf32_t *out;
    size_t outsize;

    /* Size of output string */
    outsize = 0;
    i = 0;
    while (inp[i] != 0) {
        utf32_t ch32 = inp[i++];
        if ((ch32 & 0xFC00) == 0xD800
        &&  (inp[i] & 0xFC00) == 0xDC00) {
            ++i;
        } 
        ++outsize;
    }

    out = str_alloc32(outsize);

    i = 0;
    j = 0;
    while (inp[i] != 0) {
        utf32_t ch32 = inp[i++];

        if ((ch32 & 0xFC00) == 0xD800
        &&  (inp[i] & 0xFC00) == 0xDC00) {
            ch32 = 0x10000 + ((ch32 & 0x3FF) << 10) + (inp[i++] & 0x3FF);
        } else if ((ch32 & 0xF800) == 0xD800) {
            ch32 = 0xFFFD;
        }

        out[j++] = ch32;
    }

    out[j] = 0;
    return out;
}

char *
uni_32to8(inp)
const utf32_t *inp;
{
    size_t i, j;
    char *out;
    size_t outsize;

    /* Size of output string */
    outsize = 0;
    for (i = 0; inp[i] != 0; ++i) {
        utf32_t ch32 = inp[i];
        if (ch32 > 0x10FFFF || (ch32 & 0xFFFFF800) == 0xD800) {
            ch32 = 0xFFFD;
        }
        if (ch32 < 0x80) {
            outsize += 1;
        } else if (ch32 < 0x800) {
            outsize += 2;
        } else if (ch32 < 0x10000) {
            outsize += 3;
        } else {
            outsize += 4;
        } 
    }

    out = str_alloc(outsize);

    j = 0;
    for (i = 0; inp[i] != 0; ++i) {
        utf32_t ch32 = inp[i];

        if (ch32 > 0x10FFFF || (ch32 & 0xFFFFF800) == 0xD800) {
            ch32 = 0xFFFD;
        }
        if (ch32 < 0x80) {
            out[j++] = (char) ch32;
        } else if (ch32 < 0x800) {
            out[j++] = (char) (0xC0 |  (ch32 >>  6)        );
            out[j++] = (char) (0x80 | ( ch32        & 0x3F));
        } else if (ch32 < 0x10000) {
            out[j++] = (char) (0xE0 |  (ch32 >> 12)        );
            out[j++] = (char) (0x80 | ((ch32 >>  6) & 0x3F));
            out[j++] = (char) (0x80 | ( ch32        & 0x3F));
        } else {
            out[j++] = (char) (0xF0 |  (ch32 >> 18)        );
            out[j++] = (char) (0x80 | ((ch32 >> 12) & 0x3F));
            out[j++] = (char) (0x80 | ((ch32 >>  6) & 0x3F));
            out[j++] = (char) (0x80 | ( ch32        & 0x3F));
        }
    }

    out[j] = '\0';
    return out;
}

utf16_t *
uni_32to16(inp)
const utf32_t *inp;
{
    size_t i, j;
    utf16_t *out;
    size_t outsize;

    /* Size of output string */
    outsize = 0;
    for (i = 0; inp[i] != 0; ++i) {
        utf32_t ch32 = inp[i];
        if (ch32 > 0x10FFFF || (ch32 & 0xFFFFF800) == 0xD800) {
            ch32 = 0xFFFD;
        }
        if (ch32 < 0x10000) {
            outsize += 1;
        } else {
            outsize += 2;
        } 
    }

    out = str_alloc16(outsize);

    j = 0;
    for (i = 0; inp[i] != 0; ++i) {
        utf32_t ch32 = inp[i];

        if (ch32 > 0x10FFFF || (ch32 & 0xFFFFF800) == 0xD800) {
            ch32 = 0xFFFD;
        }
        if (ch32 < 0x10000) {
            out[j++] = ch32;
        } else {
            out[j++] = 0xD7C0 + (ch32 >> 10);
            out[j++] = 0xDC00 + (ch32 & 0xFF);
        }
    }

    out[j] = 0;
    return out;
}

char *
uni_normalize8(inp, form)
const char *inp;
const char *form;
{
    str_context ctx = str_open_context("uni_normalize8");
    utf32_t *str1, *str2;
    char *str3;

    /* UTF-8 to UTF-32 */
    str1 = uni_8to32(inp);

    /* Normalize */
    str2 = uni_normalize32(str1, form);

    /* UTF-32 to UTF-8 */
    str3 = uni_32to8(str2);

    str_export(ctx, str3);
    str_close_context(ctx);
    return str3;
}

utf16_t *
uni_normalize16(inp, form)
const utf16_t *inp;
const char *form;
{
    str_context ctx = str_open_context("uni_normalize16");
    utf32_t *str1, *str2;
    utf16_t *str3;

    /* UTF-16 to UTF-32 */
    str1 = uni_16to32(inp);

    /* Normalize */
    str2 = uni_normalize32(str1, form);

    /* UTF-32 to UTF-16 */
    str3 = uni_32to16(str2);

    str_export(ctx, str3);
    str_close_context(ctx);
    return str3;
}

utf32_t *
uni_normalize32(inp, form)
const utf32_t *inp;
const char *form;
{
    utf32_t *norm = NULL;
    size_t norm_size;

    size_t i, j, k;

    /* Determine the type of normalization requested */
    boolean compose = index(form, 'C') != NULL;
    boolean compat  = index(form, 'K') != NULL;

    /* Decompose each character in inp */
    norm_size = 0;
    for (i = 0; inp[i] != 0; ++i) {
        norm_size += uni_decomposition(NULL, 0, inp[i], compat);
    }
    norm = str_alloc32(norm_size);
    j = 0;
    for (i = 0; inp[i] != 0; ++i) {
        j += uni_decomposition(norm + j, norm_size + 1 - j, inp[i], compat);
    }

    /* "Canonical sort":  sort runs of combining characters in ascending
       order of combining class.
       We can't use qsort here:  it's not guaranteed to be stable.
       Combining character sequences are assumed to be short, making
       insertion sort acceptable even though it's O(n^2). */
    i = 0;
    while (i < norm_size) {
        /* Find the next combining character */
        while (i < norm_size && uni_combining_class(norm[i]) == 0) {
            ++i;
        }
        if (i >= norm_size) {
            break;
        }

        /* Sort through the end of this run of combining characters */
        /* Outer loop invariants:  norm[i] through norm[j-1] inclusive are
           the sorted part of the run; norm[j] is the next combining
           character to be inserted into the sorted part */
        for (j = i + 1; j < norm_size; ++j) {
            utf32_t swap = norm[j];
            int cc = uni_combining_class(swap);
            if (cc == 0) {
                break;
            }

            /* Step backwards through the sorted part; if the combining
               class is greater than that of swap, move the character
               forward */
            for (k = j; k-- != i; ) {
                if (cc >= uni_combining_class(norm[k]))
                    break;
                norm[k + 1] = norm[k];
            }
            /* Insert swap in its proper place in the sorted part of the
               list */
            norm[k + 1] = swap;
        }
        i = j;
    }

    /* We're done if normalization form D or KD. */
    if (compose) {
        /* Skip any initial combining characters */
        for (i = 0; i < norm_size && uni_combining_class(norm[i]) != 0; i++) {}

        /* Look for pairs of characters that can compose */
        while (i < norm_size) {
            int last_cclass;
            utf32_t ch1;

            last_cclass = 0;
            ch1 = norm[i];
            /* ch1 is a starter; j will scan through following combining
               characters */
            for (j = i+1; j < norm_size; ++j) {
                int cclass;
                utf32_t ch2, comp;

                ch2 = norm[j];
                cclass = uni_combining_class(ch2);
                if (cclass == 0 && last_cclass != 0) {
                    break;
                }
                /* Can't compose if the previous character has the same
                   combining class, except for composing two adjacent
                   starters */
                if (cclass <= last_cclass && last_cclass != 0) {
                    last_cclass = cclass;
                    continue;
                }
                comp = uni_composition(ch1, ch2);
                if (comp != UNI_NO_CHAR) {
                    /* When composing, replace the starter with the
                       composition and the combining character with a
                       marker that is not valid Unicode */
                    norm[i] = comp;
                    norm[j] = UNI_NO_CHAR;
                    ch1 = comp;
                    /* and last_cclass stays the same */
                } else {
                    last_cclass = cclass;
                    if (cclass == 0) {
                        break;
                    }
                }
            }
            /* Advance to next starter or end of string */
            i = j;
        }

        /* Remove any markers that indicate deleted combining characters */
        j = 0;
        for (i = 0; i < norm_size; ++i) {
            if (norm[i] != UNI_NO_CHAR) {
                norm[j++] = norm[i];
            }
        }
        norm_size = j;
    }

    norm[norm_size] = 0;
    return norm;
}

size_t
uni_length16(str)
const utf16_t *str;
{
    size_t len;

    for (len = 0; str[len] != 0; ++len) {}
    return len;
}

size_t
uni_length32(str)
const utf32_t *str;
{
    size_t len;

    for (len = 0; str[len] != 0; ++len) {}
    return len;
}

size_t
uni_copy8(out, out_size, inp)
char *out;
size_t out_size;
const char *inp;
{
    size_t inp_size = strlen(inp);

    if (out != NULL && out_size != 0) {
        size_t copy_size = inp_size;

        if (copy_size > out_size - 1) {
            copy_size = out_size - 1;
        }
        /* Don't truncate in the middle of a character */
        while (copy_size != 0 && (inp[copy_size] & 0xC0) == 0x80) {
            --copy_size;
        }
        memcpy(out, inp, copy_size * sizeof(inp[0]));
        out[copy_size] = 0;
    }
    return inp_size;
}

size_t
uni_copy32(out, out_size, inp)
utf32_t *out;
size_t out_size;
const utf32_t *inp;
{
    size_t inp_size = uni_length32(inp);

    if (out != NULL && out_size != 0) {
        size_t copy_size = inp_size;

        if (copy_size > out_size - 1) {
            copy_size = out_size - 1;
        }
        memcpy(out, inp, copy_size * sizeof(inp[0]));
        out[copy_size] = 0;
    }
    return inp_size;
}

/* For comp_entry */
static int FDECL(comp_compare, (const void *, const void *));

static int
comp_compare(key_, elem_)
const genericptr key_;
const genericptr elem_;
{
    const utf32_t *key = (const utf32_t *)key_;
    const comp_entry *elem = (const comp_entry *)elem_;

    if (key[0] < elem->ch1) return -1;
    if (key[0] > elem->ch1) return +1;
    if (key[1] < elem->ch2) return -1;
    if (key[1] > elem->ch2) return +1;
    return 0;
}

static utf32_t
comp_search(table, tblen, ch1, ch2)
const comp_entry *table;
size_t tblen;
utf32_t ch1, ch2;
{
    const comp_entry *elem;
    utf32_t key[2];

    key[0] = ch1;
    key[1] = ch2;
    elem = (const comp_entry *) bsearch(
            key, table, tblen, sizeof(table[0]), comp_compare);
    return elem == NULL ? UNI_NO_CHAR : elem->comp;
}

/* For int_range_entry */
/* Returns const int_range_entry * rather than int, for use by
   unicode::decimal_value */
static int FDECL(int_range_compare, (const void *, const void *));

static int
int_range_compare(key_, elem_)
const genericptr key_;
const genericptr elem_;
{
    const utf32_t *key = (const utf32_t *)key_;
    const int_range_entry *elem = (const int_range_entry *)elem_;

    if (*key < elem->wch1) return -1;
    if (*key > elem->wch2) return +1;
    return 0;
}

static const int_range_entry *
int_range_search(table, tblen, wch)
const int_range_entry *table;
size_t tblen;
utf32_t wch;
{
    return (const int_range_entry *) bsearch(
            &wch, table, tblen, sizeof(table[0]), int_range_compare);
}

/* For string8_range_entry */
static int FDECL(string8_range_compare, (const void *, const void *));

static int
string8_range_compare(key_, elem_)
const genericptr key_;
const genericptr elem_;
{
    const utf32_t *key = (const utf32_t *) key_;
    const string8_range_entry *elem = (const string8_range_entry *) elem_;

    if (*key < elem->wch1) { return -1; }
    if (*key > elem->wch2) { return  1; }
    return 0;
}

static const char *
string8_range_search(table, tblen, wch)
const string8_range_entry *table;
size_t tblen;
utf32_t wch;
{
    const string8_range_entry *elem;

    elem = (const string8_range_entry *) bsearch(
            &wch, table, tblen, sizeof(table[0]),
            string8_range_compare);
    return elem == NULL ? NULL : elem->string;
}

/* For string_search */
static int FDECL(char_compare, (const void *, const void *));

static int
char_compare(key_, elem_)
const genericptr key_;
const genericptr elem_;
{
    const utf32_t *key = (const utf32_t *) key_;
    const char_entry *elem = (const char_entry *) elem_;

    if (*key < elem->wch) { return -1; }
    if (*key > elem->wch) { return  1; }
    return 0;
}

/* For string_search */
static int FDECL(string_compare, (const void *, const void *));

static int
string_compare(key_, elem_)
const genericptr key_;
const genericptr elem_;
{
    const utf32_t *key = (const utf32_t *) key_;
    const string_entry *elem = (const string_entry *) elem_;

    if (*key < elem->wch) { return -1; }
    if (*key > elem->wch) { return +1; }
    return 0;
}

static const utf32_t *
string_search(table_1, tb1len, table_m, tbmlen, wch)
const char_entry *table_1;
size_t tb1len;
const string_entry *table_m;
size_t tbmlen;
utf32_t wch;
{
    const char_entry *elem_1;
    const string_entry *elem_m;

    /* Search for a one-to-one mapping */
    elem_1 = (const char_entry *) bsearch(
            &wch, table_1, tb1len, sizeof(table_1[0]),
            char_compare);
    if (elem_1 != NULL)
        return elem_1->value;

    /* Search for a one-to-many mapping */
    elem_m = (const string_entry *) bsearch(
            &wch, table_m, tbmlen, sizeof(table_m[0]),
            string_compare);
    if (elem_m != NULL)
        return elem_m->string;

    return NULL;
}
