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
#include <unistd.h>
#include <ctype.h>

#include "pe.h"

int patch(int8_t **image, uint32_t *length, int32_t argc, int8_t **argv);

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "nasm-patcher patch git~%s (c) 2013 Toni Spets\n\n", REV);
        fprintf(stderr, "usage: %s <executable> [args ...]\n", argv[0]);
        return 1;
    }

    FILE *fh = fopen(argv[1], "rb");
    if (!fh)
    {
        perror("Error opening executable");
        return 1;
    }

    fseek(fh, 0L, SEEK_END);
    uint32_t length = ftell(fh);
    rewind(fh);

    int8_t *image = malloc(length);

    if (fread(image, length, 1, fh) != 1)
    {
        perror("Error reading executable");
        return 1;
    }

    int32_t ret = patch(&image, &length, argc - 2, (int8_t **)&argv[2]);

    if (ret == 0)
    {
        rewind(fh);
        fwrite(image, length, 1, fh);
    }

    fclose(fh);
    free(image);

    return ret;
}

uint32_t get_uint32(int8_t * *p)
{
    uint32_t ret = *(uint32_t *)*p;
    *p += sizeof(uint32_t);
    return ret;
}

int patch_image(int8_t *image, uint32_t address, int8_t *patch, uint32_t length)
{
    int i;

    PIMAGE_DOS_HEADER dos_hdr       = (void *)image;
    PIMAGE_NT_HEADERS nt_hdr        = (PIMAGE_NT_HEADERS)(image + dos_hdr->e_lfanew);
    PIMAGE_SECTION_HEADER sct_hdr   = IMAGE_FIRST_SECTION(nt_hdr);

    for (i = 0; i < nt_hdr->FileHeader.NumberOfSections; i++)
    {
        if (sct_hdr->VirtualAddress + nt_hdr->OptionalHeader.ImageBase <= address && address < sct_hdr->VirtualAddress + nt_hdr->OptionalHeader.ImageBase + sct_hdr->SizeOfRawData)
        {
            uint32_t offset = sct_hdr->PointerToRawData + (address - (sct_hdr->VirtualAddress + nt_hdr->OptionalHeader.ImageBase));

            if (sct_hdr->SizeOfRawData < length)
            {
                fprintf(stderr, "Error: section length (%u) is less than patch length (%d), maybe expand the image a bit more?\r\n", (unsigned int)sct_hdr->SizeOfRawData, length);
                return 0;
            }

            memcpy(image + offset, patch, length);
            printf("PATCH  %8d bytes -> %8X\r\n", length, address);

            return 1;
        }

        sct_hdr++;
    }

    fprintf(stderr, "Error: memory address %08X not found in image\r\n", address);
    return 0;
}

int patch(int8_t **image, uint32_t *length, int32_t argc, int8_t **argv)
{
    PIMAGE_DOS_HEADER dos_hdr = (void *)*image;
    PIMAGE_NT_HEADERS nt_hdr = (void *)(*image + dos_hdr->e_lfanew);
    PIMAGE_SECTION_HEADER sct_hdr = IMAGE_FIRST_SECTION(nt_hdr);
    char *section = argc > 0 ? (char *)argv[0] : "PATCH";

    if (*length < 512)
    {
        fprintf(stderr, "File too small.\n");
        return 1;
    }

    if (dos_hdr->e_magic != IMAGE_DOS_SIGNATURE)
    {
        fprintf(stderr, "File DOS signature invalid.\n");
        return 1;
    }

    if (nt_hdr->Signature != IMAGE_NT_SIGNATURE)
    {
        fprintf(stderr, "File NT signature invalid.\n");
        return 1;
    }

    int8_t *patch = NULL;
    int32_t patch_len = 0;

    for (int32_t i = 0; i < nt_hdr->FileHeader.NumberOfSections; i++)
    {
        if (strcmp(section, (char *)sct_hdr->Name) == 0)
        {
            patch = ((int8_t *)*image) + sct_hdr->PointerToRawData;
            patch_len = sct_hdr->Misc.VirtualSize;
            break;
        }

        sct_hdr++;
    }

    if (patch == NULL)
    {
        fprintf(stderr, "No '%s' section in given PE image.\n", section);
        return 1;
    }

    int8_t *p = patch;
    while (p < patch + patch_len)
    {
        uint32_t paddress = get_uint32(&p);

        if (paddress == 0)
        {
            break;
        }

        uint32_t plength = get_uint32(&p);

        if (!patch_image(*image, paddress, patch, plength))
        {
            break;
        }

        p += plength;
    }

    return 0;
}
