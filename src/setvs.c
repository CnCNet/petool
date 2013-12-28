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
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <ctype.h>
#include <unistd.h>

#include "pe.h"
#include "cleanup.h"
#include "common.h"

int setvs(int argc, char **argv)
{
    // decleration before more meaningful initialization for cleanup
    int     ret   = EXIT_SUCCESS;
    FILE   *fh    = NULL;
    int8_t *image = NULL;

    FAIL_IF(argc < 4, "usage: petool setvs <image> <section> <VirtualSize>\n");

    uint32_t vs   = strtol(argv[3], NULL, 0);

    uint32_t length;
    FAIL_IF_SILENT(open_and_read(&fh, &image, &length, argv[1], "r+b"));

    PIMAGE_DOS_HEADER dos_hdr = (void *)image;
    PIMAGE_NT_HEADERS nt_hdr = (void *)(image + dos_hdr->e_lfanew);

    FAIL_IF(length < sizeof (IMAGE_DOS_HEADER),         "File too small.\n");
    FAIL_IF(dos_hdr->e_magic != IMAGE_DOS_SIGNATURE,    "File DOS signature invalid.\n");
    FAIL_IF(dos_hdr->e_lfanew == 0,                     "NT header missing.\n");
    FAIL_IF(nt_hdr->Signature != IMAGE_NT_SIGNATURE,    "File NT signature invalid.\n");
    FAIL_IF(vs < 1,                                     "VirtualSize can't be zero.\n");

    for (int32_t i = 0; i < nt_hdr->FileHeader.NumberOfSections; i++)
    {
        PIMAGE_SECTION_HEADER sct_hdr = IMAGE_FIRST_SECTION(nt_hdr) + i;

        if (strcmp(argv[2], (char *)sct_hdr->Name) == 0)
        {
            FAIL_IF(vs < sct_hdr->SizeOfRawData,"VirtualSize can't be smaller than raw size.\n");
            sct_hdr->Misc.VirtualSize = vs;
            vs = 0;
            break;
        }
    }

    FAIL_IF(vs != 0, "No '%s' section in given PE image.\n", argv[2]);

    /* FIXME: implement checksum calculation */
    nt_hdr->OptionalHeader.CheckSum = 0;

    rewind(fh);
    FAIL_IF_PERROR(fwrite(image, length, 1, fh) != 1, "Error writing executable");

cleanup:
    if (image) free(image);
    if (fh)    fclose(fh);
    return ret;
}
