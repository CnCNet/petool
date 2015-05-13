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

int genlds(int argc, char **argv)
{
    // decleration before more meaningful initialization for cleanup
    int     ret   = EXIT_SUCCESS;
    FILE   *fh    = NULL;
    FILE   *ofh   = stdout;
    int8_t *image = NULL;

    FAIL_IF(argc < 2, "usage: petool genlds <image> [ofile]\n");

    uint32_t length;
    FAIL_IF_SILENT(open_and_read(&fh, &image, &length, argv[1], "rb"));

    if (argc > 2)
    {
        FAIL_IF(file_exists(argv[2]), "%s: output file already exists.\n", argv[2]);
        ofh = fopen(argv[2], "w");
        FAIL_IF_PERROR(ofh == NULL, "%s");
    }

    fclose(fh);
    fh = NULL; // for cleanup

    PIMAGE_DOS_HEADER dos_hdr = (void *)image;
    PIMAGE_NT_HEADERS nt_hdr = (void *)(image + dos_hdr->e_lfanew);

    FAIL_IF(length < 512,                            "File too small.\n");
    FAIL_IF(dos_hdr->e_magic != IMAGE_DOS_SIGNATURE, "File DOS signature invalid.\n");
    FAIL_IF(nt_hdr->Signature != IMAGE_NT_SIGNATURE, "File NT signature invalid.\n");

    fprintf(ofh, "/* GNU ld linker script for %s */\n", file_basename(argv[1]));
    fprintf(ofh, "start = 0x%"PRIX32";\n", nt_hdr->OptionalHeader.ImageBase + nt_hdr->OptionalHeader.AddressOfEntryPoint);
    fprintf(ofh, "ENTRY(start);\n");
    fprintf(ofh, "SECTIONS\n");
    fprintf(ofh, "{\n");

    bool idata_exists = false;
    char bss_name[10] = { '\0' };

    for (int i = 0; i < nt_hdr->FileHeader.NumberOfSections; i++)
    {
        const PIMAGE_SECTION_HEADER cur_sct = IMAGE_FIRST_SECTION(nt_hdr) + i;
        char buf[9];
        memset(buf, 0, sizeof buf);
        memcpy(buf, cur_sct->Name, 8);

        if (cur_sct->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA) {
            fprintf(ofh, "    %-15s   0x%-6"PRIX32" : { . = . + 0x%"PRIX32"; }\n", buf, cur_sct->VirtualAddress + nt_hdr->OptionalHeader.ImageBase, cur_sct->Misc.VirtualSize ? cur_sct->Misc.VirtualSize : cur_sct->SizeOfRawData);
            strcpy(bss_name, buf);
            continue;
        }

        /* resource section is not always directly recompilable even if it doesn't move, better to leave it out */
        if (strcmp(buf, ".rsrc") == 0) {
            fprintf(ofh, "    /DISCARD/                  : { %s(%s) }\n", argv[1], buf);
            continue;
        }

        fprintf(ofh, "    %-15s   0x%-6"PRIX32" : { %s(%s) }\n", buf, cur_sct->VirtualAddress + nt_hdr->OptionalHeader.ImageBase, file_basename(argv[1]), buf);

        if (cur_sct->Misc.VirtualSize > cur_sct->SizeOfRawData) {
            fprintf(ofh, "    .bss         ALIGN(0x%-4"PRIX32") : { . = . + 0x%"PRIX32"; }\n", nt_hdr->OptionalHeader.SectionAlignment, cur_sct->Misc.VirtualSize - cur_sct->SizeOfRawData);
        }

        if (strcmp(buf, ".idata") == 0) {
            idata_exists = true;
        }
    }

    fprintf(ofh, "\n");

    if (!idata_exists) {
        fprintf(ofh, "    .idata       ALIGN(0x%-4"PRIX32") : { *(.idata); }\n\n", nt_hdr->OptionalHeader.SectionAlignment);
    }

    if (bss_name[0]) {
        fprintf(ofh, "    /DISCARD/                  : { %s(%s) }\n", argv[1], bss_name);
    }

    fprintf(ofh, "    /DISCARD/                  : { *(.drectve) }\n");
    fprintf(ofh, "    .p_text      ALIGN(0x%-4"PRIX32") : { *(.text); }\n", nt_hdr->OptionalHeader.SectionAlignment);
    fprintf(ofh, "    .p_rdata     ALIGN(0x%-4"PRIX32") : { *(.rdata); }\n", nt_hdr->OptionalHeader.SectionAlignment);
    fprintf(ofh, "    .p_data      ALIGN(0x%-4"PRIX32") : { *(.data); }\n\n", nt_hdr->OptionalHeader.SectionAlignment);
    fprintf(ofh, "    .p_bss       ALIGN(0x%-4"PRIX32") : { *(.bss) *(COMMON); }\n\n", nt_hdr->OptionalHeader.SectionAlignment);
    fprintf(ofh, "    .patch       ALIGN(0x%-4"PRIX32") : { *(.patch) }\n", nt_hdr->OptionalHeader.SectionAlignment);

    fprintf(ofh, "}\n");

cleanup:
    if (image) free(image);
    if (fh)    fclose(fh);
    if (argc > 2)
    {
        if (ofh)   fclose(ofh);
    }
    return ret;
}
