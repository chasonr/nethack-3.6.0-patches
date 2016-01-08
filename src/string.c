/* A dynamic string allocator with simple garbage collection */

#include "hack.h"

/* This header precedes any allocation */
struct str_record
{
    struct str_record *next, *prev;
    struct str_context_rec *ctx;
};

/* Complete this type */
struct str_context_rec
{
    struct str_context_rec *next;
    const char *trace;
    struct str_record *head; /* circular linked list */
};

/* Contexts are kept on a stack. When any string allocation function is called,
   it receives a context, and the string is allocated into that context. When a
   context is closed, the strings allocated into it are freed.

   If you forget to close a context, closing the next one (or calling
   str_clear, which closes all contexts except the global one) will close that
   context and issue a paniclog warning. */

/* The global context, for use when no context was opened; always at the bottom
   of the stack */
static struct str_context_rec global_ctx = {
    NULL, "global", NULL
};

/* The current context */
static str_context top_ctx = &global_ctx;

static void FDECL(pop_context, (BOOLEAN_P));
static void FDECL(pop_to_context, (str_context));
static void NDECL(clear_context);
static void FDECL(attach_record, (str_context, struct str_record *));
static void FDECL(detach_record, (struct str_record *));

/* Free all string contexts and empty the global context */
void
str_clear()
{
    pop_to_context(&global_ctx);
    clear_context();
}

/* Open a string context */
/* trace should be a string constant, such as the name of the calling function.
  No copy is made, and it must last until the context is closed. */
str_context
str_open_context(trace)
const char *trace;
{
    /* Create a new empty context */
    str_context ctx = (str_context) alloc(sizeof(*ctx));
    ctx->trace = trace;
    ctx->head = NULL;

    /* Place it at the top of the stack */
    ctx->next = top_ctx;
    top_ctx = ctx;

    return ctx;
}

/* Close a string context; also close any nested contexts left open, and warn
   that they were not closed */
void
str_close_context(ctx)
str_context ctx;
{
    if (ctx == NULL || ctx == &global_ctx) {
        /* The global context is never closed, only emptied */
        str_clear();
    } else {
        pop_to_context(ctx);
        pop_context(FALSE);
    }
}

/* Allocate storage of the given size */
genericptr_t
str_mem_alloc(size)
size_t size;
{
    struct str_record *rec;

    rec = (struct str_record *) alloc(size + sizeof(*rec));
    attach_record(top_ctx, rec);

    return (genericptr_t) (rec + 1);    
}

/* Allocate storage for a string of the given length */
char *
str_alloc(size)
size_t size;
{
    return str_mem_alloc(size + 1);
}

/* Allocate storage for a string of the given length */
utf16_t *
str_alloc16(size)
size_t size;
{
    return str_mem_alloc((size + 1) * sizeof(utf16_t));
}

/* Allocate storage for a string of the given length */
utf32_t *
str_alloc32(size)
size_t size;
{
    return str_mem_alloc((size + 1) * sizeof(utf32_t));
}

/* Create a copy of an 8 bit string */
char *
str_copy(str)
const char *str;
{
    return strcpy(str_alloc(strlen(str)), str);
}

/* Create a copy of a 16 bit string */
utf16_t *
str_copy16(str)
const utf16_t *str;
{
    size_t size;
    utf16_t *mem;

    for (size = 0; str[size] != 0; ++size) {}
    mem = str_alloc16(size);
    memcpy(mem, str, (size + 1) * sizeof(str[0]));
    return mem;
}

/* Create a copy of a 32 bit string */
utf32_t *
str_copy32(str)
const utf32_t *str;
{
    size_t size;
    utf32_t *mem;

    for (size = 0; str[size] != 0; ++size) {}
    mem = str_alloc32(size);
    memcpy(mem, str, (size + 1) * sizeof(str[0]));
    return mem;
}

/* Export a string, so that it survives when the context is closed */
/* Note that ctx must not be NULL -- exporting from the global context is
   undefined */
genericptr_t
str_export(ctx, mem)
str_context ctx;
genericptr_t mem;
{
    struct str_record *rec = (struct str_record *)mem - 1;

    /* Detach the record from the context it's in */
    detach_record(rec);
    /* Attach to the context enclosing this one */
    attach_record(ctx->next, rec);

    return mem;
}

/* Convert allocator storage to regular malloc'd storage */
char *
str_detach(str)
char *str;
{
    struct str_record *rec = (struct str_record *)str - 1;
    char *str2 = (char *)rec;

    /* Detach the record from the context it's in */
    detach_record(rec);

    /* Copy over the header and return */
    /* Use memmove, because the source and destination may overlap */
    memmove(str2, str, strlen(str) + 1);

    return str2;
}

/* Convert allocator storage to regular malloc'd storage */
utf16_t *
str_detach16(str)
utf16_t *str;
{
    struct str_record *rec = (struct str_record *)str - 1;
    utf16_t *str2 = (utf16_t *)rec;
    size_t size;

    /* Detach the record from the context it's in */
    detach_record(rec);

    /* Copy over the header and return */
    /* Use memmove, because the source and destination may overlap */
    for (size = 0; str[size] != 0; ++size) {}
    memmove(str2, str, (size + 1) * sizeof(str2[0]));

    return str2;
}

/* Convert allocator storage to regular malloc'd storage */
utf32_t *
str_detach32(str)
utf32_t *str;
{
    struct str_record *rec = (struct str_record *)str - 1;
    utf32_t *str2 = (utf32_t *)rec;
    size_t size;

    /* Detach the record from the context it's in */
    detach_record(rec);

    /* Copy over the header and return */
    /* Use memmove, because the source and destination may overlap */
    for (size = 0; str[size] != 0; ++size) {}
    memmove(str2, str, (size + 1) * sizeof(str2[0]));

    return str2;
}

/* Local function: clear and pop the top context off the stack */
static void
pop_context(warn)
boolean warn;
{
    str_context ctx = top_ctx;

    if (warn) {
        char buf[BUFSZ];

        Sprintf(buf, "String context '%s' was not closed", ctx->trace);
        paniclog("Memory", buf);
    }

    clear_context();
    top_ctx = ctx->next;
    free(ctx);
}

/* Local function: clear and pop contexts nested within the given one */
static void
pop_to_context(ctx)
str_context ctx;
{
    while (top_ctx != ctx) {
        pop_context(TRUE);
    }
}

/* Local function: free strings in the topmost context */
static void
clear_context()
{
    if (top_ctx->head != NULL) {
        struct str_record *p = top_ctx->head;
        p->prev->next = NULL;
        while (p != NULL) {
            struct str_record *q = p->next;
            free(p);
            p = q;
        }
        top_ctx->head = NULL;
    }
}

/* Local function: attach a record to a context */
static void
attach_record(ctx, rec)
str_context ctx;
struct str_record *rec;
{
    if (ctx->head == NULL) {
        rec->next = rec;
        rec->prev = rec;
        ctx->head = rec;
    } else {
        rec->next = ctx->head;
        rec->prev = ctx->head->prev;
        rec->prev->next = rec;
        rec->next->prev = rec;
    }
    rec->ctx = ctx;
}

static void
detach_record(rec)
struct str_record *rec;
{
    str_context ctx = rec->ctx;

    if (ctx->head == rec) {
        ctx->head = rec->next;
    }
    if (ctx->head == rec) {
        ctx->head = NULL;
    } else {
        rec->prev->next = rec->next;
        rec->next->prev = rec->prev;
    }
}
