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
#include <strings.h>
#include <ctype.h>
#include <unistd.h>
#include "pe.h"

struct annotation {
    char *type;
    int argc;
    char **argv; 
};

static struct annotation annotations[512];

char *read_stream(FILE *stream)
{
    char *buf;
    int i, len = 1024, pos = 0;

    if ((buf = malloc(len + 1)) == NULL)
    {
        fprintf(stderr, "malloc: out of memory\n");
        return NULL;
    }

    buf[pos] = '\0';

    while ((i = fgetc(stream)) != EOF)
    {
        if (pos == len)
        {
            len += 1024;
            if ((buf = realloc(buf, len + 1)) == NULL)
            {
                fprintf(stderr, "realloc: out of memory\n");
                return NULL;
            }
        }

        buf[pos++] = (char)i;
    }

    buf[pos] = 0;

    return buf;
}

struct annotation *parse_annotation(const char *str)
{
    int i, tok = -1, len;
    struct annotation *ann;

    if (*str != '@')
    {
        return NULL;
    }

    ann = calloc(1, sizeof (struct annotation));

    len = strlen(++str);

    for (i = 0; i <= len; i++)
    {
        if (!isspace(str[i]) && tok == -1)
        {
            tok = i;
        }
        else if (tok > -1 && !isspace(str[i-1]) && (i == len || isspace(str[i])))
        {
            if (ann->type == NULL)
            {
                int j;
                ann->type = calloc(1, i - tok + 1);
                for (j = 0; j < i - tok; j++)
                    ann->type[j] = tolower(*(str + tok + j));
            }
            else
            {
                ann->argc++;
                ann->argv = realloc(ann->argv, ann->argc * sizeof(char *));

                if (ann->argv)
                {
                    ann->argv[ann->argc-1] = calloc(1, i - tok + 1);
                    memcpy(ann->argv[ann->argc-1], str + tok, i - tok);
                }
                else
                {
                    /* out of memory */
                    ann->argc = 0;
                }
            }

            tok = -1;
        }
    }

    if (ann->type)
    {
        return ann;
    }

    free(ann);
    return NULL;
}

void get_annotations(const char *src)
{
    int num = 0;
    int pos = 0, len = strlen(src), tok = -1;

    for (; pos < len; pos++)
    {
        if (tok == -1 && src[pos] == '@' && (pos == 0 || src[pos-1] == '\n' || (pos > 1 && src[pos-2] == '\n' && isspace(src[pos-1]))))
        {
            tok = pos;
        }

        if (tok > -1 && (src[pos] == '\r' || src[pos] == '\n'))
        {
            struct annotation *a;
            char *ann = malloc((pos - tok) + 1);
            memcpy(ann, src + tok, pos - tok);
            memset((void *)(src + tok), ' ', pos - tok);
            ann[pos - tok] = '\0';

            a = parse_annotation(ann);
            if (a)
            {
                memcpy(&annotations[num], a, sizeof annotations[num]);
                free(a);
                num++;
            }

            free(ann);
            tok = -1;
        }
    }
}

void update_annotations(const char *k, unsigned int v)
{
    int i = 0;
    struct annotation *ann;
    do {
        ann = &annotations[i++];
        if (ann->argc > 0)
        {
            int j;
            for (j = 0; j < ann->argc; j++)
            {
                if (strcmp(ann->argv[j], k) == 0)
                {
                    free(ann->argv[j]);
                    ann->argv[j] = calloc(1, 11);
                    sprintf(ann->argv[j], "0x%08X", v);
                }
            }
        }
    } while (ann->type);
}

int patch_image(void *image, unsigned int address, void *patch, int length)
{
    int i;

    PIMAGE_DOS_HEADER dos_hdr       = image;
    PIMAGE_NT_HEADERS nt_hdr        = (PIMAGE_NT_HEADERS)((char *)image + dos_hdr->e_lfanew);
    PIMAGE_SECTION_HEADER sct_hdr   = IMAGE_FIRST_SECTION(nt_hdr);

    for (i = 0; i < nt_hdr->FileHeader.NumberOfSections; i++)
    {
        if (sct_hdr->VirtualAddress + nt_hdr->OptionalHeader.ImageBase <= address && address < sct_hdr->VirtualAddress + nt_hdr->OptionalHeader.ImageBase + sct_hdr->SizeOfRawData)
        {
            unsigned int offset = sct_hdr->PointerToRawData + (address - (sct_hdr->VirtualAddress + nt_hdr->OptionalHeader.ImageBase));

            memcpy((char *)image + offset, patch, length);

            return 1;
        }

        sct_hdr++;
    }

    return 0;
}

int main(int argc, char **argv)
{
    FILE *exe;
    FILE *patch;
    unsigned int exe_length;
    unsigned int patch_length;
    char *exe_data;
    char *patch_data;

    char label[256];
    unsigned int address = 0;
    unsigned int a;
    char *l;
    int i;
    char *map_buf;
    FILE *fh;
    char *source;
    char *nasm = "nasm";
    char nasm_flags[1024] = { '\0' };
    char *tmp_name;
    char *out_name;
    char *inc_name;
    char *exe_name;
    char buf[1024];

    memset(&annotations, 0, sizeof annotations);

    if (argc < 3)
    {
        fprintf(stderr, "linker for ra303p git~%s (c) 2012 Toni Spets\n\n", REV);
        fprintf(stderr, "usage: %s <source file> <out include file> <target executable> [nasm [nasm flags]]\n", argv[0]);
        return 1;
    }

    inc_name = argv[2];
    exe_name = argv[3];
    tmp_name = tempnam(NULL, "lnkr-");
    out_name = tempnam(NULL, "lnkr-");

    if (argc > 3)
    {
        nasm = argv[4];
    }

    if (argc > 4)
    {
        int i = 5;
        for (; i < argc; i++)
        {
            strncat(nasm_flags, " ", sizeof nasm_flags);
            strncat(nasm_flags, argv[i], sizeof nasm_flags);
        }
    }

    snprintf(buf, 1024, "%s%s -e %s", nasm, nasm_flags, argv[1]);

    if ((fh = popen(buf, "r")) == NULL)
    {
        fprintf(stderr, "%s\n", buf);
        perror(nasm);
        return 1;
    }

    source = read_stream(fh);

    if (pclose(fh) != 0)
    {
        return 1;
    }

    get_annotations(source);

    if ((fh = fopen(tmp_name, "w")) == NULL)
    {
        perror(tmp_name);
        return 1;
    }

    /* always working with 32 bit images and need to map all for linking */
    fprintf(fh, "[bits 32]\n[map all]\n");
    fwrite(source, strlen(source), 1, fh);
    fclose(fh);

    snprintf(buf, 1024, "%s%s -f bin %s -o %s", nasm, nasm_flags, tmp_name, out_name);

    if ((fh = popen(buf, "r")) == NULL)
    {
        fprintf(stderr, "%s\n", buf);
        perror(nasm);
        unlink(tmp_name);
        return 1;
    }

    map_buf = read_stream(fh);

    if (pclose(fh) != 0)
    {
        unlink(tmp_name);
        return 1;
    }

    unlink(tmp_name);

    fh = fopen(inc_name, "w");
    fprintf(fh, "; generated by linker\r\n");

    l = strtok(map_buf, "\n");
    i = 0;
    do {
        if (l == NULL)
            break;

        if (address == 0)
            sscanf(l, "%X", &address);

        if (strstr(l, "Section .text"))
            i = 1;

        if (i && sscanf(l, "%*X %X  %s", &a, label) == 2)
        {
            if (strstr(l, ".") == NULL)
            {
                fprintf(fh, "%%define %-32s 0x%08X\r\n", label, a);
                update_annotations(label, a);
            }
        }
    } while((l = strtok(NULL, "\n")));

    fclose(fh);

    free(map_buf);

    if (!address)
    {
        fprintf(stderr, "linker: ORG not found in map\n");
        return 1;
    }

    exe = fopen(exe_name, "rb+");
    if (!exe)
    {
        perror(exe_name);
        return 1;
    }

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

    patch = fopen(out_name, "rb");
    if (!patch)
    {
        fclose(exe);
        perror(out_name);
        unlink(out_name);
        return 1;
    }

    fseek(patch, 0L, SEEK_END);
    patch_length = ftell(patch);
    rewind(patch);

    /* ignore empty patches */
    if (patch_length == 0)
    {
        free(exe_data);
        fclose(exe);
        return 0;
    }

    patch_data = malloc(patch_length);

    if (fread(patch_data, patch_length, 1, patch) != 1)
    {
        fclose(exe);
        fclose(patch);
        unlink(out_name);
        perror("linker: error reading patch");
        return 1;
    }

    fclose(patch);
    unlink(out_name);

    if (!patch_image(exe_data, (unsigned int)address, patch_data, patch_length))
    {
        fprintf(stderr, "linker: memory address 0x%08X not found in %s\n", (unsigned int)address, exe_name);
        return 1;
    }

    printf("PATCH  %8d bytes -> %8X\n", patch_length, address);

    /* create annotation patches */
    for (i = 0; i < 512; i++)
    {
        struct annotation *ann = &annotations[i];
        if (ann->type == NULL)
            break;

        if ((strcmp(ann->type, "hook") == 0 || strcmp(ann->type, "jmp") == 0) && ann->argc >= 2)
        {
            unsigned int from = strtol(ann->argv[0], NULL, 0);
            unsigned int to = strtol(ann->argv[1], NULL, 0);

            if (abs(to - from) < 128)
            {
                unsigned char buf[] = { 0xEB, 0x00 };
                *(signed char *)(buf + 1) = to - from - 2;
                if (!patch_image(exe_data, from, buf, sizeof buf))
                {
                    fprintf(stderr, "linker: memory address 0x%08X not found in %s\n", (unsigned int)address, exe_name);
                    return 1;
                }

                printf("%-12s %8X -> %8X\n", "JMP SHORT", from, to);
            }
            else
            {
                unsigned char buf[] = { 0xE9, 0x00, 0x00, 0x00, 0x00 };
                *(signed int *)(buf + 1) = to - from - 5;
                if (!patch_image(exe_data, from, buf, sizeof buf))
                {
                    fprintf(stderr, "linker: memory address 0x%08X not found in %s\n", (unsigned int)address, exe_name);
                    return 1;
                }

                printf("%-12s %8X -> %8X\n", "JMP", from, to);
            }
        }
        else if (strcmp(ann->type, "call") == 0 && ann->argc >= 2)
        {
            unsigned int from = strtol(ann->argv[0], NULL, 0);
            unsigned int to = strtol(ann->argv[1], NULL, 0);
            unsigned char buf[] = { 0xE8, 0x00, 0x00, 0x00, 0x00 };
            *(signed int *)(buf + 1) = to - from - 5;
            if (!patch_image(exe_data, from, buf, sizeof buf))
            {
                fprintf(stderr, "linker: memory address 0x%08X not found in %s\n", (unsigned int)address, exe_name);
                return 1;
            }

            printf("%-12s %8X -> %8X\n", "CALL", from, to);
        }
        else if (strcmp(ann->type, "clear") == 0 && ann->argc >= 3)
        {
            unsigned int from = strtol(ann->argv[0], NULL, 0);
            unsigned int character = strtol(ann->argv[1], NULL, 0);
            unsigned int to = strtol(ann->argv[2], NULL, 0);
            unsigned int length = to - from;
            char *zbuf = malloc(length);

            if (!zbuf)
            {
                fprintf(stderr, "linker: out of memory when allocating %d for zbuf\n", length);
                return 1;
            }

            memset(zbuf, (char)character, length);

            if (!patch_image(exe_data, from, zbuf, length))
            {
                fprintf(stderr, "linker: memory address 0x%08X not found in %s\n", (unsigned int)from, exe_name);
                return 1;
            }

            free(zbuf);

            printf("CLEAR  %8d bytes -> %8X\n", length, from);
        }
        else
        {
            fprintf(stderr, "linker: warning: unknown annotation \"%s\"\n", ann->type);
        }
    }

    if (fwrite(exe_data, exe_length, 1, exe) != 1)
    {
        perror("linker: error writing to executable file");
        return 1;
    }

    free(exe_data);
    free(patch_data);

    fclose(exe);

    return 0;
}
