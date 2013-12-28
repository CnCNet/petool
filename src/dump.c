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

    NO_FAIL(argc < 2, "usage: petool dump <image>\n");

    uint32_t length;
    NO_FAIL_SILENT(open_and_read(&fh, &image, &length, argv[1], "rb"));

    fclose(fh);
    fh = NULL; // for cleanup

    PIMAGE_DOS_HEADER dos_hdr = (void *)image;
    PIMAGE_NT_HEADERS nt_hdr = (void *)(image + dos_hdr->e_lfanew);

    NO_FAIL(length < 512,                            "File too small.\n");
    NO_FAIL(dos_hdr->e_magic != IMAGE_DOS_SIGNATURE, "File DOS signature invalid.\n");
    NO_FAIL(nt_hdr->Signature != IMAGE_NT_SIGNATURE, "File NT signature invalid.\n");

    printf(" section    start      end   length    vaddr    vsize  flags\n");
    printf("------------------------------------------------------------\n");

    for (int i = 0; i < nt_hdr->FileHeader.NumberOfSections; i++)
    {
        const PIMAGE_SECTION_HEADER cur_sct = IMAGE_FIRST_SECTION(nt_hdr) + i;
        printf(
            "%8.8s %8"PRIu32" %8"PRIu32" %8"PRIu32" %8"PRIX32" %8"PRIu32" %c%c%c%c%c%c\n",
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
