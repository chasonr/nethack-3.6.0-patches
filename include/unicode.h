/* unicode.h -- common transformations on Unicode strings */

#ifndef UNICODE_H
#define UNICODE_H

#include "tradstdc.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UNI_NO_CHAR 0xFFFFFFFF

/* Character properties */
size_t FDECL(uni_decomposition, (
        utf32_t *out, size_t out_size,
        utf32_t wch,
        BOOLEAN_P compat));
utf32_t FDECL(uni_composition, (utf32_t wch1, utf32_t wch2));
int FDECL(uni_combining_class, (utf32_t wch));

/* Case mappings */
int FDECL(str_casecmp, (const char *s1, const char *s2));

/* Transforms */
utf16_t *FDECL(uni_8to16,  (const char *inp));
utf32_t *FDECL(uni_8to32,  (const char *inp));
char    *FDECL(uni_16to8,  (const utf16_t *inp));
utf32_t *FDECL(uni_16to32, (const utf16_t *inp));
char *   FDECL(uni_32to8,  (const utf32_t *inp));
utf16_t *FDECL(uni_32to16, (const utf32_t *inp));

char *FDECL(uni_normalize8, (const char *inp, const char *form));
utf16_t *FDECL(uni_normalize16, (const utf16_t *inp, const char *form));
utf32_t *FDECL(uni_normalize32, (const utf32_t *inp, const char *form));

/* String functions */
size_t uni_length16(const utf16_t *str);
size_t uni_length32(const utf32_t *str);
size_t uni_copy8(char *out, size_t out_size, const char *inp);
size_t uni_copy16(utf16_t *out, size_t out_size, const utf16_t *inp);
size_t uni_copy32(utf32_t *out, size_t out_size, const utf32_t *inp);

#ifdef __cplusplus
}
#endif

#endif
