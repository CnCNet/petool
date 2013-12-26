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
#include <inttypes.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>

#include "pe.h"

int pe2obj(void **image, long *length, int argc, char **argv);

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "nasm-patcher pe2obj git~%s (c) 2012-2013 Toni Spets\n\n", REV);
        fprintf(stderr, "usage: %s <executable> [args ...]\n", argv[0]);
        return EXIT_FAILURE;
    }

    FILE *fh = fopen(argv[1], "rb");
    if (!fh)
    {
        perror("Error opening executable");
        return EXIT_FAILURE;
    }

    fseek(fh, 0L, SEEK_END);
    long length = ftell(fh);
    rewind(fh);

    void *image = malloc(length);

    if (fread(image, length, 1, fh) != 1)
    {
        perror("Error reading executable");
        return EXIT_FAILURE;
    }

    fclose(fh);

    int ret = pe2obj(&image, &length, argc - 2, &argv[2]);

    free(image);

    return ret;
}

int pe2obj(void **image, long *length, int argc, char **argv)
{
    PIMAGE_DOS_HEADER dos_hdr = *image;
    PIMAGE_NT_HEADERS nt_hdr = (void *)((char *)*image + dos_hdr->e_lfanew);

    if (*length < 512)
    {
        fprintf(stderr, "File too small.\n");
        return EXIT_FAILURE;
    }

    if (dos_hdr->e_magic != IMAGE_DOS_SIGNATURE)
    {
        fprintf(stderr, "File DOS signature invalid.\n");
        return EXIT_FAILURE;
    }

    if (nt_hdr->Signature != IMAGE_NT_SIGNATURE)
    {
        fprintf(stderr, "File NT signature invalid.\n");
        return EXIT_FAILURE;
    }

    printf("   section    start      end   length    vaddr  flags\n");
    printf("-----------------------------------------------------\n");

    for (int i = 0; i < nt_hdr->FileHeader.NumberOfSections; i++)
    {
        const PIMAGE_SECTION_HEADER cur_sct = IMAGE_FIRST_SECTION(nt_hdr) + i;
        printf(
            "%10.8s %8"PRIu32" %8"PRIu32" %8"PRIu32" %8"PRIX32" %c%c%c%c%c%c\n",
            cur_sct->Name,
            cur_sct->PointerToRawData,
            cur_sct->PointerToRawData + cur_sct->SizeOfRawData,
            cur_sct->SizeOfRawData ? cur_sct->SizeOfRawData : cur_sct->Misc.VirtualSize,
            cur_sct->VirtualAddress + nt_hdr->OptionalHeader.ImageBase,
            cur_sct->Characteristics & IMAGE_SCN_MEM_READ               ? 'r' : '-',
            cur_sct->Characteristics & IMAGE_SCN_MEM_WRITE              ? 'w' : '-',
            cur_sct->Characteristics & IMAGE_SCN_MEM_EXECUTE            ? 'x' : '-',
            cur_sct->Characteristics & IMAGE_SCN_CNT_CODE               ? 'c' : '-',
            cur_sct->Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA   ? 'i' : '-',
            cur_sct->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA ? 'u' : '-'
        );

        if (cur_sct->PointerToRawData)
        {
            cur_sct->PointerToRawData -= dos_hdr->e_lfanew + 4;
        }
    }

    if (argc < 1)
    {
        fprintf(stderr, "No output file");
        return EXIT_FAILURE;
    }

    FILE *fh = fopen(argv[0], "wb");
    fwrite((char *)*image + (dos_hdr->e_lfanew + 4), *length - (dos_hdr->e_lfanew + 4), 1, fh);
    fclose(fh);

    printf("Wrote object to %s\n", argv[0]);

    return EXIT_SUCCESS;
}
