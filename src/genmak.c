/*
 * Copyright (c) 2015 Toni Spets <toni.spets@iki.fi>
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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cleanup.h"
#include "common.h"

int genmak(int argc, char **argv)
{
    int     ret   = EXIT_SUCCESS;
    FILE   *ofh   = stdout;
    char   base[256] = { '\0' };

    FAIL_IF(argc < 2, "usage: petool genmak <image> [ofile]\n");

    if (argc > 2)
    {
        FAIL_IF(file_exists(argv[2]), "%s: output file already exists.\n", argv[2]);
        ofh = fopen(argv[2], "w");
        FAIL_IF_PERROR(ofh == NULL, "%s");
    }

    strcpy(base, file_basename(argv[1]));
    char *p = strrchr(base, '.');
    if (p)
    {
        *p = '\0';
    }

    fprintf(ofh, "-include config.mk\n\n");
    fprintf(ofh, "INPUT       = %s\n", argv[1]);
    fprintf(ofh, "OUTPUT      = %sp.exe\n", base);
    fprintf(ofh, "LDS         = %sp.lds\n", base);
    fprintf(ofh, "LDFLAGS     = --subsystem=windows\n\n"); // FIXME: detect subsystem from input file header?

    fprintf(ofh, "OBJS        = rsrc.o\n\n");

    fprintf(ofh, "PETOOL     ?= petool\n");
    fprintf(ofh, "STRIP      ?= strip\n\n");

    fprintf(ofh, "all: $(OUTPUT)\n\n");

    fprintf(ofh, "rsrc.o: $(INPUT)\n");
    fprintf(ofh, "\t$(PETOOL) re2obj $(INPUT) $@\n\n");

    fprintf(ofh, "$(OUTPUT): $(LDS) $(INPUT) $(OBJS)\n");
    fprintf(ofh, "\t$(LD) $(LDFLAGS) -T $(LDS) -o $@ $(INPUT) $(OBJS)\n");
    fprintf(ofh, "\t$(PETOOL) patch $@ || ($(RM) $@ && exit 1)\n");
    fprintf(ofh, "\t$(STRIP) -R .patch $@ || ($(RM) $@ && exit 1)\n");
    fprintf(ofh, "\t$(PETOOL) dump $@\n\n");

    fprintf(ofh, "clean:\n");
    fprintf(ofh, "\t$(RM) $(OUTPUT) $(OBJS)\n");

cleanup:
    if (argc > 2)
    {
        if (ofh)   fclose(ofh);
    }

    return ret;
}
