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
#include <time.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int pe2obj(void **image, long *length, int argc, char **argv);

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "nasm-patcher pe2obj git~%s (c) 2012-2013 Toni Spets\n\n", REV);
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
    long length = ftell(fh);
    rewind(fh);

    void *image = malloc(length);

    if (fread(image, length, 1, fh) != 1)
    {
        perror("Error reading executable");
        return 1;
    }

    fclose(fh);

    int ret = pe2obj(&image, &length, argc - 2, &argv[2]);

    free(image);

    return ret;
}

int strflags(char *flag_list)
{
    int flags = 0;

    for (int i = 0; i < strlen(flag_list); i++)
    {
        switch (tolower(flag_list[i]))
        {
            case 'r': flags |= IMAGE_SCN_MEM_READ;                  break;
            case 'w': flags |= IMAGE_SCN_MEM_WRITE;                 break;
            case 'x': flags |= IMAGE_SCN_MEM_EXECUTE;               break;
            case 'c': flags |= IMAGE_SCN_CNT_CODE;                  break;
            case 'i': flags |= IMAGE_SCN_CNT_INITIALIZED_DATA;      break;
            case 'u': flags |= IMAGE_SCN_CNT_UNINITIALIZED_DATA;    break;
        }
    }

    return flags;
}

int pe2obj(void **image, long *length, int argc, char **argv)
{
    PIMAGE_DOS_HEADER dos_hdr = *image;
    PIMAGE_NT_HEADERS nt_hdr = (void *)((char *)*image + dos_hdr->e_lfanew);
    PIMAGE_SECTION_HEADER sct_hdr = IMAGE_FIRST_SECTION(nt_hdr);

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

    printf("   section    start      end   length    vaddr  flags\n");
    printf("-----------------------------------------------------\n");

    for (int i = 0; i < nt_hdr->FileHeader.NumberOfSections; i++)
    {
        printf("%10.8s %8u %8u %8u %8X %s%s%s%s%s%s\n",
                sct_hdr->Name,
                (unsigned int)sct_hdr->PointerToRawData,
                (unsigned int)(sct_hdr->PointerToRawData + sct_hdr->SizeOfRawData),
                (unsigned int)(sct_hdr->SizeOfRawData ? sct_hdr->SizeOfRawData : sct_hdr->Misc.VirtualSize),
                (unsigned int)(sct_hdr->VirtualAddress + nt_hdr->OptionalHeader.ImageBase),
                sct_hdr->Characteristics & IMAGE_SCN_MEM_READ               ? "r" : "-",
                sct_hdr->Characteristics & IMAGE_SCN_MEM_WRITE              ? "w" : "-",
                sct_hdr->Characteristics & IMAGE_SCN_MEM_EXECUTE            ? "x" : "-",
                sct_hdr->Characteristics & IMAGE_SCN_CNT_CODE               ? "c" : "-",
                sct_hdr->Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA   ? "i" : "-",
                sct_hdr->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA ? "u" : "-"
        );

        if (sct_hdr->PointerToRawData)
        {
            sct_hdr->PointerToRawData -= dos_hdr->e_lfanew + 4;
        }

        sct_hdr++;
    }

    if (argc < 1)
    {
        fprintf(stderr, "No output file");
        return 1;
    }

    FILE *fh = fopen(argv[0], "wb");
    fwrite((char *)*image + (dos_hdr->e_lfanew + 4), *length - (dos_hdr->e_lfanew + 4), 1, fh);
    fclose(fh);

    printf("Wrote object to %s\n", argv[0]);

    return 0;
}
