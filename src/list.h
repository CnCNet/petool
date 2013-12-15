/*
 * Copyright (c) 2011, 2012, 2013 Toni Spets <toni.spets@iki.fi>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdlib.h>

#define LIST_ALLOC(type)                                    \
    calloc(1, sizeof(type))

#define LIST_INSERT(list, el)                               \
    (el)->next = (list);                                    \
    (list) = (el);                                          \

#define LIST_FOREACH(list, type, el)                        \
    for (type *el = (list); el != NULL; el = (type*)el->next)

#define LIST_REMOVE(list, el)                               \
    if ((list) == (el))                                     \
    {                                                       \
        (list) = (el)->next;                                \
    }                                                       \
    else                                                    \
    {                                                       \
        void *_eltmp = (el);                                \
        (el) = (list);                                      \
        do {                                                \
            if ((el)->next == _eltmp)                       \
            {                                               \
                (el)->next = (el)->next->next;              \
                (el) = _eltmp;                              \
                break;                                      \
            }                                               \
            (el) = (el)->next;                              \
        } while(el);                                        \
        (el) = _eltmp;                                      \
    }

#define LIST_FREE(el)                                       \
    while (el)                                              \
    {                                                       \
        void *_eltmp = (el);                                \
        (el) = (el)->next;                                  \
        free(_eltmp);                                       \
    }                                                       \
    (el) = NULL
