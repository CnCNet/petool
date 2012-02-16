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
#include <string.h>
#include "pe.h"

int main(int argc, char **argv)
{
    unsigned int address;
    FILE *exe;
    FILE *patch;
    unsigned int exe_length;
    unsigned int patch_length;
    char *exe_data;
    char *patch_data;
    int i;

    PIMAGE_DOS_HEADER dos_hdr;
    PIMAGE_NT_HEADERS nt_hdr;
    PIMAGE_SECTION_HEADER sct_hdr;

    if (argc < 4)
    {
        fprintf(stderr, "usage: %s <executable> <address> <patch>\n", argv[0]);
        return 1;
    }

    exe = fopen(argv[1], "rb+");
    if (!exe)
    {
        perror("linker: failed to open executable file");
        return 1;
    }

    address = strtol(argv[2], NULL, 0);

    fseek(exe, 0L, SEEK_END);
    exe_length = ftell(exe);
    rewind(exe);

    exe_data = malloc(exe_length);

    if (fread(exe_data, exe_length, 1, exe) != 1)
    {
        fclose(exe);
        perror("linker: error reading executable");
        return 1;
    }

    rewind(exe);

    patch = fopen(argv[3], "rb");
    if (!patch)
    {
        fclose(exe);
        perror("linker: failed to open patch file");
        return 1;
    }

    fseek(patch, 0L, SEEK_END);
    patch_length = ftell(patch);
    rewind(patch);

    patch_data = malloc(patch_length);

    if (fread(patch_data, patch_length, 1, patch) != 1)
    {
        fclose(exe);
        perror("linker: error reading executable");
        return 1;
    }

    fclose(patch);

    dos_hdr = (void *)exe_data;
    nt_hdr = (void *)(exe_data + dos_hdr->e_lfanew);
    sct_hdr = IMAGE_FIRST_SECTION(nt_hdr);

    for (i = 0; i < nt_hdr->FileHeader.NumberOfSections; i++)
    {
        if (sct_hdr->VirtualAddress + nt_hdr->OptionalHeader.ImageBase <= address && address < sct_hdr->VirtualAddress + nt_hdr->OptionalHeader.ImageBase + sct_hdr->SizeOfRawData)
        {
            unsigned int offset = sct_hdr->PointerToRawData + (address - (sct_hdr->VirtualAddress + nt_hdr->OptionalHeader.ImageBase));

            printf("linker: %8d bytes -> 0x%08X\n", patch_length, offset);

            memcpy(exe_data + offset, patch_data, patch_length);

            if (fwrite(exe_data, exe_length, 1, exe) != 1)
            {
                perror("linker: error writing to executable file");
                return 1;
            }

            goto cleanup;
        }
        sct_hdr++;
    }

    fprintf(stderr, "linker: memory address 0x%08X not found in %s\n", (unsigned int)address, argv[1]);
    return 1;

cleanup:

    free(exe_data);
    free(patch_data);

    fclose(exe);

    return 0;
}
