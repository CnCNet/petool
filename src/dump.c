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
#include <stdbool.h>
#include <inttypes.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>

#include "pe.h"
#include "cleanup.h"
#include "common.h"

int dump(int argc, char **argv)
{
    // decleration before more meaningful initialization for cleanup
    int     ret   = EXIT_SUCCESS;
    FILE   *fh    = NULL;
    int8_t *image = NULL;

    FAIL_IF(argc < 2, "usage: petool dump <image>\n");

    uint32_t length;
    FAIL_IF_SILENT(open_and_read(&fh, &image, &length, argv[1], "rb"));

    fclose(fh);
    fh = NULL; // for cleanup

    PIMAGE_DOS_HEADER dos_hdr = (void *)image;
    PIMAGE_NT_HEADERS nt_hdr = (void *)(image + dos_hdr->e_lfanew);

    if (nt_hdr->Signature != IMAGE_NT_SIGNATURE)
    {
        if (dos_hdr->e_magic == IMAGE_DOS_SIGNATURE)
        {
            uint32_t exe_start = dos_hdr->e_cparhdr * 16L;
            uint32_t exe_end = dos_hdr->e_cp * 512L - (dos_hdr->e_cblp ? 512L - dos_hdr->e_cblp : 0);

            printf("DOS Header:\n");
            printf(" e_magic:    %04X\n", dos_hdr->e_magic);
            printf(" e_cblp:     %04X\n", dos_hdr->e_cblp);
            printf(" e_cp:       %04X\n", dos_hdr->e_cp);
            printf(" e_crlc:     %04X\n", dos_hdr->e_crlc);
            printf(" e_cparhdr:  %04X\n", dos_hdr->e_cparhdr);
            printf(" e_minalloc: %04X\n", dos_hdr->e_minalloc);
            printf(" e_maxalloc: %04X\n", dos_hdr->e_maxalloc);
            printf(" e_ss:       %04X\n", dos_hdr->e_ss);
            printf(" e_sp:       %04X\n", dos_hdr->e_sp);
            printf(" e_csum:     %04X\n", dos_hdr->e_csum);
            printf(" e_ip:       %04X\n", dos_hdr->e_ip);
            printf(" e_cs:       %04X\n", dos_hdr->e_cs);
            printf(" e_lfarlc:   %04X\n", dos_hdr->e_lfarlc);
            printf(" e_ovno:     %04X\n", dos_hdr->e_ovno);

            printf("\nEXE data is from offset %04X (%d) to %04X (%d).\n", exe_start, exe_start, exe_end, exe_end);
            goto cleanup;
        }

        // nasty trick but we're careful, right?
        nt_hdr = (void *)(image - 4);
        if (nt_hdr->FileHeader.Machine != 0x014C)
        {
            fprintf(stderr, "No valid signatures found in input file.\n");
            goto cleanup;
        }
    }
    else
    {
        FAIL_IF(length < 512, "File too small.\n");
    }

    printf(" section    start      end   length    vaddr    vsize  flags\n");
    printf("------------------------------------------------------------\n");

    for (int i = 0; i < nt_hdr->FileHeader.NumberOfSections; i++)
    {
        const PIMAGE_SECTION_HEADER cur_sct = IMAGE_FIRST_SECTION(nt_hdr) + i;
        printf(
            "%8.8s %8"PRIX32" %8"PRIX32" %8"PRIX32" %8"PRIX32" %8"PRIX32" %c%c%c%c%c%c\n",
            cur_sct->Name,
            cur_sct->PointerToRawData,
            cur_sct->PointerToRawData + cur_sct->SizeOfRawData,
            cur_sct->SizeOfRawData,
            cur_sct->VirtualAddress + nt_hdr->OptionalHeader.ImageBase,
            cur_sct->Misc.VirtualSize,
            cur_sct->Characteristics & IMAGE_SCN_MEM_READ               ? 'r' : '-',
            cur_sct->Characteristics & IMAGE_SCN_MEM_WRITE              ? 'w' : '-',
            cur_sct->Characteristics & IMAGE_SCN_MEM_EXECUTE            ? 'x' : '-',
            cur_sct->Characteristics & IMAGE_SCN_CNT_CODE               ? 'c' : '-',
            cur_sct->Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA   ? 'i' : '-',
            cur_sct->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA ? 'u' : '-'
        );
    }

    if (nt_hdr->OptionalHeader.NumberOfRvaAndSizes >= 2)
    {
        printf("Import Table: %8"PRIX32" (%"PRIu32" bytes)\n", nt_hdr->OptionalHeader.DataDirectory[1].VirtualAddress, nt_hdr->OptionalHeader.DataDirectory[1].Size);
    }

cleanup:
    if (image) free(image);
    if (fh)    fclose(fh);
    return ret;
}
