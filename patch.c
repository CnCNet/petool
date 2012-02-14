/*
 * Copyright (c) 2012 Toni Spets <toni.spets@iki.fi>
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

int main(int argc, char **argv)
{
    FILE *exe;
    FILE *bin;
    long offset;
    long length;
    char *tmp;

    if (argc < 4)
    {
        fprintf(stderr, "usage: %s <executable> <address> <file>\n", argv[0]);
        return 1;
    }

    exe = fopen(argv[1], "r+");
    if (!exe)
    {
        perror("Failed to open executable file");
        return 1;
    }

    offset = atol(argv[2]);

    if (offset < 0)
    {
        fprintf(stderr, "Invalid offset %ld", offset);
        return 1;
    }

    bin = fopen(argv[3], "r");
    if (!bin)
    {
        fclose(exe);
        perror("Failed to open binary file");
        return 1;
    }

    fseek(bin, 0L, SEEK_END);
    length = ftell(bin);
    rewind(bin);

    tmp = malloc(length);

    printf("Patching %ld bytes at offset 0x%08X\n", length, (unsigned int)offset);

    if (fread(tmp, length, 1, bin) != 1)
    {
        perror("Error reading from binary file");
        return 1;
    }

    if (fseek(exe, offset, SEEK_SET) != 0)
    {
        perror("Seeking the executable failed");
        return 1;
    }

    if (fwrite(tmp, length, 1, exe) != 1)
    {
        perror("Error writing to executable file");
        return 1;
    }

    free(tmp);

    fclose(exe);
    fclose(bin);

    return 0;
}
