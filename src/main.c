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
#include <stdlib.h>
#include <string.h>

int dump(int argc, char **argv);
int genlds(int argc, char **argv);
int pe2obj(int argc, char **argv);
int patch(int argc, char **argv);
int setdd(int argc, char **argv);
int setvs(int argc, char **argv);
int export(int argc, char **argv);
int import(int argc, char **argv);
int re2obj(int argc, char **argv);
int genmak(int argc, char **argv);

void help(char *progname)
{
    fprintf(stderr, "petool git~%s (c) 2013 Toni Spets\n\n", REV);
    fprintf(stderr, "usage: %s <command> [args ...]\n\n", progname);
    fprintf(stderr, "commands:"                                                 "\n"
            "    dump   -- dump information about section of executable"        "\n"
            "    genlds -- generate GNU ld script for re-linking executable"    "\n"
            "    pe2obj -- convert PE executable into win32 object file"        "\n"
            "    patch  -- apply a patch set from the .patch section"           "\n"
            "    setdd  -- set any DataDirectory in PE header"                  "\n"
            "    setvs  -- set VirtualSize for a section"                       "\n"
            "    export -- export section data as raw binary"                   "\n"
            "    import -- dump the import table as assembly"                   "\n"
            "    re2obj -- convert the resource section into COFF object"       "\n"
            "    genmak -- generate project Makefile"                           "\n"
            "    help   -- this information"                                    "\n"
    );
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "No command given: please give valid command name as first argument\n\n");
        help(argv[0]);
        return EXIT_FAILURE;
    }
    else if (strcmp(argv[1], "dump")   == 0) return dump   (argc - 1, argv + 1);
    else if (strcmp(argv[1], "genlds") == 0) return genlds (argc - 1, argv + 1);
    else if (strcmp(argv[1], "pe2obj") == 0) return pe2obj (argc - 1, argv + 1);
    else if (strcmp(argv[1], "patch")  == 0) return patch  (argc - 1, argv + 1);
    else if (strcmp(argv[1], "setdd")  == 0) return setdd  (argc - 1, argv + 1);
    else if (strcmp(argv[1], "setvs")  == 0) return setvs  (argc - 1, argv + 1);
    else if (strcmp(argv[1], "export") == 0) return export (argc - 1, argv + 1);
    else if (strcmp(argv[1], "import") == 0) return import (argc - 1, argv + 1);
    else if (strcmp(argv[1], "re2obj") == 0) return re2obj (argc - 1, argv + 1);
    else if (strcmp(argv[1], "genmak") == 0) return genmak (argc - 1, argv + 1);
    else if (strcmp(argv[1], "help")   == 0)
    {
        help(argv[0]);
        return EXIT_SUCCESS;
    }
    else
    {
        fprintf(stderr, "Unknown command: %s\n", argv[1]);
        help(argv[0]);
        return EXIT_FAILURE;
    }
}
