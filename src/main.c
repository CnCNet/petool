/*
 * Copyright (c) 2013 Toni Spets <toni.spets@iki.fi>
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

#include <stdio.h>
#include <string.h>

int pe2obj(int argc, char **argv);
int patch(int argc, char **argv);

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "petool git~%s (c) 2013 Toni Spets\n\n", REV);
        fprintf(stderr, "usage: %s <command> [args ...]\n", argv[0]);
        fprintf(stderr, "commands: pe2obj patch\n");
        return 1;
    }

    int ret = 1;

    if (strcmp(argv[1], "pe2obj") == 0)
    {
        ret = pe2obj(argc - 1, &argv[1]);
    }
    else if (strcmp(argv[1], "patch") == 0)
    {
        ret = patch(argc - 1, &argv[1]);
    }
    else
    {
        fprintf(stderr, "Unknown command: %s\n", argv[1]);
    }

    return ret;
}
