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

int patch_image(int8_t *image, uint32_t address, int8_t *patch, uint32_t length)
{
    PIMAGE_DOS_HEADER dos_hdr       = (void *)image;
    PIMAGE_NT_HEADERS nt_hdr        = (PIMAGE_NT_HEADERS)(image + dos_hdr->e_lfanew);

    for (int i = 0; i < nt_hdr->FileHeader.NumberOfSections; i++)
    {
        PIMAGE_SECTION_HEADER sct_hdr = IMAGE_FIRST_SECTION(nt_hdr) + i;

        if (sct_hdr->VirtualAddress + nt_hdr->OptionalHeader.ImageBase <= address && address < sct_hdr->VirtualAddress + nt_hdr->OptionalHeader.ImageBase + sct_hdr->SizeOfRawData)
        {
            uint32_t offset = sct_hdr->PointerToRawData + (address - (sct_hdr->VirtualAddress + nt_hdr->OptionalHeader.ImageBase));

            if (sct_hdr->SizeOfRawData < length)
            {
                fprintf(stderr, "Error: section length (%"PRIu32") is less than patch length (%"PRId32"), maybe expand the image a bit more?\n", sct_hdr->SizeOfRawData, length);
                return EXIT_SUCCESS;
            }

            memcpy(image + offset, patch, length);
            printf("PATCH  %8"PRId32" bytes -> %8"PRIX32"\n", length, address);
            return EXIT_FAILURE;
        }
    }

    fprintf(stderr, "Error: memory address %08"PRIX32" not found in image\n", address);
    return EXIT_SUCCESS;
}

uint32_t get_uint32(int8_t * *p)
{
    uint32_t ret = *(uint32_t *)*p;
    *p += sizeof(uint32_t);
    return ret;
}

int patch(int argc, char **argv)
{
    // decleration before more meaningful initialization for cleanup
    int     ret   = EXIT_SUCCESS;
    FILE   *fh    = NULL;
    int8_t *image = NULL;

    NO_FAIL(argc < 2, "usage: petool patch <image> [section]\n");

    uint32_t length;
    NO_FAIL_SILENT(open_and_read(&fh, &image, &length, argv[1], "r+b"));

    PIMAGE_DOS_HEADER dos_hdr = (void *)image;
    PIMAGE_NT_HEADERS nt_hdr = (void *)(image + dos_hdr->e_lfanew);

    char *section = argc > 2 ? (char *)argv[2] : ".patch";
    int8_t *patch = NULL;
    int32_t patch_len = 0;

    for (int32_t i = 0; i < nt_hdr->FileHeader.NumberOfSections; i++)
    {
        PIMAGE_SECTION_HEADER sct_hdr = IMAGE_FIRST_SECTION(nt_hdr) + i;

        if (strcmp(section, (char *)sct_hdr->Name) == 0)
        {
            patch = image + sct_hdr->PointerToRawData;
            patch_len = sct_hdr->Misc.VirtualSize;
            break;
        }

        sct_hdr++;
    }

    NO_FAIL(patch == NULL, "No '%s' section in given PE image.\n", section);

    for (int8_t *p = patch; p < patch + patch_len;)
    {
        uint32_t paddress = get_uint32(&p);
        if (paddress == 0)                             break;

        uint32_t plength = get_uint32(&p);
        if (!patch_image(image, paddress, p, plength)) break;

        p += plength;
    }

    rewind(fh);
    NO_FAIL_PERROR(fwrite(image, length, 1, fh) != 1, "Error writing executable");

cleanup:
    if (image) free(image);
    if (fh)    fclose(fh);
    return ret;
}
