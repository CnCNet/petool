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
#include "modpe.h"

int main(int argc, char **argv)
{
    FILE *fh;
    char *data;
    unsigned int bytes;
    long length;
    int i;

    PIMAGE_DOS_HEADER dos_hdr;
    PIMAGE_NT_HEADERS nt_hdr;
    PIMAGE_SECTION_HEADER sct_hdr;

    if (argc < 3)
    {
        fprintf(stderr, "usage: %s <executable> <bytes>\n", argv[0]);
        return 1;
    }

    fh = fopen(argv[1], "r+");
    if (!fh)
    {
        perror("Error opening executable");
        return 1;
    }

    bytes = atoi(argv[2]);

    fseek(fh, 0L, SEEK_END);
    length = ftell(fh);
    rewind(fh);

    data = malloc(length);
    fread(data, length, 1, fh);
    rewind(fh);

    length += bytes;

    dos_hdr = (void *)data;
    nt_hdr = (void *)(data + dos_hdr->e_lfanew);
    sct_hdr = IMAGE_FIRST_SECTION(nt_hdr);

    for (i = 0; i < nt_hdr->FileHeader.NumberOfSections; i++)
    {
        if (strncmp(sct_hdr->Name, "AUTO", 6) == 0)
        {
            /* give write access to original code */
            sct_hdr->Characteristics |= IMAGE_SCN_MEM_WRITE;
        }

        sct_hdr++;
    }

    sct_hdr--;
    sct_hdr->Characteristics |= IMAGE_SCN_MEM_EXECUTE|IMAGE_SCN_MEM_READ|IMAGE_SCN_MEM_WRITE;

    printf("File offset for binary data is %d\n", sct_hdr->PointerToRawData + sct_hdr->SizeOfRawData);

    /* move stuff outta our space */
    memcpy(data + sct_hdr->PointerToRawData + sct_hdr->SizeOfRawData, data + sct_hdr->PointerToRawData + sct_hdr->SizeOfRawData + bytes, (data + length) - (data + sct_hdr->PointerToRawData + sct_hdr->SizeOfRawData + bytes));

    /* initialize our new code block to INT3 */
    memset(data + sct_hdr->PointerToRawData + sct_hdr->SizeOfRawData, 0xCC, bytes);

    /* give all permissions for the memory area */
    sct_hdr->Characteristics |= IMAGE_SCN_MEM_EXECUTE|IMAGE_SCN_MEM_READ|IMAGE_SCN_MEM_WRITE;

    nt_hdr->OptionalHeader.AddressOfEntryPoint = sct_hdr->VirtualAddress + sct_hdr->SizeOfRawData;
    sct_hdr->SizeOfRawData += bytes;

    printf("New entry point at 0x%08X\n", nt_hdr->OptionalHeader.ImageBase + nt_hdr->OptionalHeader.AddressOfEntryPoint, bytes);

    fwrite(data, length, 1, fh);

    free(data);
    fclose(fh);

    return 0;
}
