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
#include <inttypes.h>

#ifdef _WIN32
#include <direct.h>
#define MAX_PATH 260 /* including windows.h would conflict with pe.h */
#endif

#ifdef __linux__
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/limits.h>
#define MAX_PATH PATH_MAX
#define _mkdir(a) mkdir(a, 0777)
#endif

#include "pe.h"
#include "cleanup.h"
#include "common.h"

int genlds(int argc, char **argv);
int genmak(int argc, char **argv);

int genprj(int argc, char **argv)
{
    int ret = EXIT_SUCCESS;
    static char base[MAX_PATH];
    static char buf[MAX_PATH];
    static char dir[MAX_PATH];
    char *cmd_argv[3] = { argv[0], argv[1], buf };

    FAIL_IF(argc < 2, "usage: petool genprj <image> [directory]\n");
    FAIL_IF(!file_exists(argv[1]), "input file missing\n");

    memset(base, 0, sizeof base);
    strncpy(base, file_basename(argv[1]), sizeof(base) - 1);
    char *p = strrchr(base, '.');
    if (p)
    {
        *p = '\0';
    }

    if (argc > 2)
    {
        memset(dir, 0, sizeof dir);
        strncpy(dir, argv[2], sizeof dir - 1);
    }
    else
    {
        snprintf(dir, sizeof dir, "%sp", base);
    }

    printf("Input file      : %s\n", argv[1]);
    printf("Output directory: %s\n", dir);

    FAIL_IF_PERROR(_mkdir(dir) == -1, "Failed to create output directory");

    snprintf(buf, sizeof buf, "%s/%s", dir, file_basename(argv[1]));
    printf("Copying %s -> %s...\n", argv[1], buf);
    FAIL_IF(file_copy(argv[1], buf) != EXIT_SUCCESS, "Failed to copy file\n");

    snprintf(buf, sizeof buf, "%s/%sp.lds", dir, base);
    printf("Generating %s...\n", buf);
    FAIL_IF(genlds(3, cmd_argv) != EXIT_SUCCESS, "Failed to create linker script\n");

    snprintf(buf, sizeof buf, "%s/Makefile", dir);
    printf("Generating %s...\n", buf);
    FAIL_IF(genmak(3, cmd_argv) != EXIT_SUCCESS, "Failed to create Makefile\n");

cleanup:
    return ret;
}
