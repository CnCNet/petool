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
    FILE *fh;
    unsigned char *data;
    long length;
    int i;
    unsigned int flags = 0;
    char *name = NULL;

    PIMAGE_DOS_HEADER dos_hdr;
    PIMAGE_NT_HEADERS nt_hdr;
    PIMAGE_SECTION_HEADER sct_hdr;

    if (argc < 4)
    {
        fprintf(stderr, "nasm-patcher modpe git~%s (c) 2012-2013 Toni Spets\n\n", REV);
        fprintf(stderr, "usage: %s <executable> <section name> <flags: rwxciu>\n", argv[0]);
        return 1;
    }

    name = argv[2];

    if (strlen(name) > 8)
    {
        fprintf(stderr, "Error: section name over 8 characters.\n");
        return 1;
    }

    for (i = 0; i < strlen(argv[3]); i++)
    {
        switch (argv[3][i])
        {
            case 'r': flags |= IMAGE_SCN_MEM_READ;                  break;
            case 'w': flags |= IMAGE_SCN_MEM_WRITE;                 break;
            case 'x': flags |= IMAGE_SCN_MEM_EXECUTE;               break;
            case 'c': flags |= IMAGE_SCN_CNT_CODE;                  break;
            case 'i': flags |= IMAGE_SCN_CNT_INITIALIZED_DATA;      break;
            case 'u': flags |= IMAGE_SCN_CNT_UNINITIALIZED_DATA;    break;
        }
    }

    fh = fopen(argv[1], "rb+");
    if (!fh)
    {
        perror("Error opening executable");
        return 1;
    }

    fseek(fh, 0L, SEEK_END);
    length = ftell(fh);
    rewind(fh);

    data = malloc(length);

    if (fread(data, length, 1, fh) != 1)
    {
        fclose(fh);
        perror("Error reading executable");
        return 1;
    }

    rewind(fh);

    dos_hdr = (void *)data;
    nt_hdr = (void *)(data + dos_hdr->e_lfanew);
    sct_hdr = IMAGE_FIRST_SECTION(nt_hdr);

    printf("   section    start      end   length    vaddr  flags\n");
    printf("-----------------------------------------------------\n");

    for (i = 0; i < nt_hdr->FileHeader.NumberOfSections; i++)
    {
        char sct_name[9];
        memset(sct_name, 0, sizeof name);
        memcpy(sct_name, sct_hdr->Name, 8);

        if (strcmp(sct_name, name) == 0)
        {
            sct_hdr->Characteristics = flags;
            nt_hdr->OptionalHeader.CheckSum     = 0x00000000;
        }

        printf("%10.8s %8d %8d %8d %8X %s%s%s%s%s%s %s\n",
                sct_hdr->Name,
                sct_hdr->PointerToRawData,
                sct_hdr->PointerToRawData + sct_hdr->SizeOfRawData,
                sct_hdr->SizeOfRawData ? sct_hdr->SizeOfRawData : sct_hdr->Misc.VirtualSize,
                sct_hdr->VirtualAddress + nt_hdr->OptionalHeader.ImageBase,
                sct_hdr->Characteristics & IMAGE_SCN_MEM_READ               ? "r" : "-",
                sct_hdr->Characteristics & IMAGE_SCN_MEM_WRITE              ? "w" : "-",
                sct_hdr->Characteristics & IMAGE_SCN_MEM_EXECUTE            ? "x" : "-",
                sct_hdr->Characteristics & IMAGE_SCN_CNT_CODE               ? "c" : "-",
                sct_hdr->Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA   ? "i" : "-",
                sct_hdr->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA ? "u" : "-",
                strcmp(sct_name, name) == 0                                 ? "<- this was updated" : ""
        );

        sct_hdr++;
    }

    fwrite(data, length, 1, fh);
    free(data);

    fclose(fh);

    return 0;
}
