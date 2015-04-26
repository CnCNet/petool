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

uint32_t rva_to_offset(uint32_t address, PIMAGE_NT_HEADERS nt_hdr)
{
    for (int i = 0; i < nt_hdr->FileHeader.NumberOfSections; i++)
    {
        PIMAGE_SECTION_HEADER sct_hdr = IMAGE_FIRST_SECTION(nt_hdr) + i;

        if (sct_hdr->VirtualAddress + nt_hdr->OptionalHeader.ImageBase <= address && address < sct_hdr->VirtualAddress + nt_hdr->OptionalHeader.ImageBase + sct_hdr->SizeOfRawData)
        {
            return sct_hdr->PointerToRawData + (address - (sct_hdr->VirtualAddress + nt_hdr->OptionalHeader.ImageBase));
        }
    }

    return 0;
}

int import(int argc, char **argv)
{
    // decleration before more meaningful initialization for cleanup
    int     ret   = EXIT_SUCCESS;
    FILE   *fh    = NULL;
    int8_t *image = NULL;

    FAIL_IF(argc < 2, "usage: petool import <image> [nasm]\n");

    uint32_t length;
    FAIL_IF_SILENT(open_and_read(&fh, &image, &length, argv[1], "rb"));

    PIMAGE_DOS_HEADER dos_hdr = (void *)image;
    PIMAGE_NT_HEADERS nt_hdr = (void *)(image + dos_hdr->e_lfanew);

    FAIL_IF (nt_hdr->OptionalHeader.NumberOfRvaAndSizes < 2, "Not enough DataDirectories.\n");

    uint32_t offset = rva_to_offset(nt_hdr->OptionalHeader.ImageBase + nt_hdr->OptionalHeader.DataDirectory[1].VirtualAddress, nt_hdr);
    IMAGE_IMPORT_DESCRIPTOR *i = (void *)(image + offset);

    if (argc > 2 && toupper(argv[2][0]) == 'N') {
        printf("; Imports for %s\n", argv[1]);
        printf("ImageBase equ 0x%"PRIX32"\n", nt_hdr->OptionalHeader.ImageBase);
        printf("\n");
        printf("section .idata\n\n");

        while (1) {
            if (i->Name != 0) {
                char *name = (char *)(image + rva_to_offset(nt_hdr->OptionalHeader.ImageBase + i->Name, nt_hdr));
                printf("; %s\n", name);
            } else {
                printf("; END\n");
            }

            printf("dd 0x%-8"PRIX32" ; OriginalFirstThunk\n", i->OriginalFirstThunk);
            printf("dd 0x%-8"PRIX32" ; TimeDateStamp\n", i->TimeDateStamp);
            printf("dd 0x%-8"PRIX32" ; ForwarderChain\n", i->ForwarderChain);
            printf("dd 0x%-8"PRIX32" ; Name\n", i->Name);
            printf("dd 0x%-8"PRIX32" ; FirstThunk\n", i->FirstThunk);

            printf("\n");

            if (i->OriginalFirstThunk == 0)
                break;
            i++;
        }
    } else {
        printf("/* Imports for %s */\n", argv[1]);
        printf(".equ ImageBase, 0x%"PRIX32"\n", nt_hdr->OptionalHeader.ImageBase);
        printf("\n");
        printf(".section .idata\n\n");

        while (1) {
            if (i->Name != 0) {
                char *name = (char *)(image + rva_to_offset(nt_hdr->OptionalHeader.ImageBase + i->Name, nt_hdr));
                printf("/* %s */\n", name);
            } else {
                printf("/* END */\n");
            }

            printf(".long 0x%-8"PRIX32" /* OriginalFirstThunk */\n", i->OriginalFirstThunk);
            printf(".long 0x%-8"PRIX32" /* TimeDateStamp */\n", i->TimeDateStamp);
            printf(".long 0x%-8"PRIX32" /* ForwarderChain */\n", i->ForwarderChain);
            printf(".long 0x%-8"PRIX32" /* Name */\n", i->Name);
            printf(".long 0x%-8"PRIX32" /* FirstThunk */\n", i->FirstThunk);

            printf("\n");

            if (i->OriginalFirstThunk == 0)
                break;
            i++;
        }
    }

cleanup:
    if (image) free(image);
    return ret;
}
