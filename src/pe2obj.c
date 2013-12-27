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
#include <stdint.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>

#include "pe.h"
#include "cleanup.h"

int pe2obj(int argc, char **argv)
{
    // decleration before more meaningful initialization for cleanup
    int     ret   = EXIT_SUCCESS;
    FILE   *fh   = NULL;
    int8_t *image = NULL;

    ENSURE(argc != 3, "usage: petool pe2obj <in> <out>\n");

    fh = fopen(argv[1], "rb");
    ENSURE_PERROR(!fh, "Error opening executable");

    fseek(fh, 0L, SEEK_END);
    uint32_t length = ftell(fh);
    rewind(fh);

    image = malloc(length);

    ENSURE_PERROR(fread(image, length, 1, fh) != 1, "Error reading executable");

    fclose(fh);
    fh = NULL; // for cleanup;

    PIMAGE_DOS_HEADER dos_hdr = (void *)image;
    PIMAGE_NT_HEADERS nt_hdr = (void *)(image + dos_hdr->e_lfanew);

    ENSURE(length < 512,                            "File too small.\n");
    ENSURE(dos_hdr->e_magic != IMAGE_DOS_SIGNATURE, "File DOS signature invalid.\n");
    ENSURE(nt_hdr->Signature != IMAGE_NT_SIGNATURE, "File NT signature invalid.\n");

    for (int i = 0; i < nt_hdr->FileHeader.NumberOfSections; i++)
    {
        const PIMAGE_SECTION_HEADER cur_sct = IMAGE_FIRST_SECTION(nt_hdr) + i;
        if (cur_sct->PointerToRawData)
        {
            cur_sct->PointerToRawData -= dos_hdr->e_lfanew + 4;
        }

        if (cur_sct->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA && !(cur_sct->Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA))
        {
            cur_sct->PointerToRawData = 0;
            cur_sct->SizeOfRawData = 0;
        }
    }

    fh = fopen(argv[2], "wb");
    ENSURE_PERROR(!fh, "Error opening output file");

    ENSURE_PERROR(fwrite((char *)image + (dos_hdr->e_lfanew + 4),
			 length - (dos_hdr->e_lfanew + 4),
			 1,
			 fh) != 1,
		  "Failed to write object file to output file\n");

cleanup:
    if (image) free(image);
    if (fh)    fclose(fh);
    return ret;
}
