#ifdef WIN32
#define  _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <wchar.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <sys/stat.h>
#include "zlib/zconf.h"
#include "zlib/zlib.h"

const static wchar_t key[] = { L'@', L'G', L'a', L'w', L'^', L'2', 
                               L't', L'G', L'Q', L'6', L'1', L'-', 
                               L'Î', L'Ò', L'n', L'i'};

static unsigned char result[1024*1024];

int sncasecmp(char *s1, char *s2, size_t n)
{
    unsigned int  c1, c2;

    while (n) {
        c1 = (unsigned int)*s1++;
        c2 = (unsigned int)*s2++;

        c1 = (c1 >= 'A' && c1 <= 'Z') ? (c1 | 0x20) : c1;
        c2 = (c2 >= 'A' && c2 <= 'Z') ? (c2 | 0x20) : c2;

        if (c1 == c2) {

            if (c1) {
                n--;
                continue;
            }

            return 0;
        }

        return c1 - c2;
    }

    return 0;
}

int decompression(unsigned char *src, size_t srcsize, unsigned char *dst, size_t *dstsize)
{
    *dstsize = 1024 * 1024;
    if (Z_OK != uncompress(result, dstsize, src, srcsize))
        return -1;

    return 0;
}

int isfilter(char *tok, int len) 
{
    if (!sncasecmp(tok, "[id", 3)) {
        return 1;
    } else if (!sncasecmp(tok, "[by", 3)) {
        return 1;
    } else if (!sncasecmp(tok, "[hash", 5)) {
        return 1;
    } else if (!sncasecmp(tok, "[al", 3)) {
        return 1;
    } else if (!sncasecmp(tok, "[sign", 5)) {
        return 1;
    } else if (!sncasecmp(tok, "[total", 6)) {
        return 1;
    } else if (!sncasecmp(tok, "[offset", 7)) {
        return 1;
    }

    return 0;
}

int create_lrc(char *path, unsigned char *lrc, size_t lrclen)
{
    size_t i;
    FILE *fp;
    int top = 0;
    int j;

    fp = fopen(path, "wb");
    if (!fp) {
        fprintf(stderr, "%s", strerror(errno));
        return -1;
    }

    for (i = 0; i < lrclen; i++) {
        int len;
        if (top == 0) {
            switch (lrc[i]) {
            case '<':
                top++;
                break;
            case '[':
                len = (strchr((char*)&lrc[i], ']') - (char*)&lrc[i]) + 1;

                for (j = 0; j < len; j++) {
                    if (lrc[i+j] == ':') {
                        if (isfilter((char*)&lrc[i], len)) {
                            while (lrc[++i] != '\n' && i < lrclen) {
                            }
                        }
                        goto filter_done;
                    }
                }

                for (j = 0; j < len; j++) {
                    int ms;

                    if (lrc[i + j] == ',') {
                        char ftime[14];
                        lrc[i + j] = 0;

                        ms = atoi((char*)&lrc[i + 1]);
                        sprintf(ftime, "[%.2d:%.2d.%.2d]", (ms % (1000 * 60 * 60)) / (1000 * 60), (ms % (1000 * 60)) / 1000, (ms % (1000 * 60)) % 100);
                      
                        for (j = 0; j < 10; j++) {
                            fputc(ftime[j], fp);
                        }

                        i = i + len - 1;
                        break;
                    }
                }
                break;
filter_done: 
            default:
                fputc(lrc[i], fp);
                break;
            }

        } else if (top == 1 && lrc[i] == '>') {
            top--;
        }
    }

    fclose(fp);

    return 0;
}

int main(int argc, char **argv)
{
    int i;
    FILE *fp;
    struct stat st;
    unsigned char *src;
    size_t dstsize;

    if (argc < 2) {
        printf("usage1: krc2lrc test.krc test.lrc\nusage2: krc2lrc test.krc\n");
        return -1;
    }

    fp = fopen(argv[1], "rb");
    if (!fp) {
        fprintf(stderr, "%s", strerror(errno));
        return -1;
    }

    if (fstat(fileno(fp), &st)) {
        fprintf(stderr, "%s", strerror(errno));
        return -1;
    }
    
    src = (unsigned char *) malloc(st.st_size);
    if (!src) {
        fprintf(stderr, "%s", strerror(errno));
        return -1;
    }
    
    if (fread(src, sizeof(unsigned char), st.st_size, fp) != st.st_size) {
        fprintf(stderr, "%s", strerror(errno));
        return -1;
    }

    if (memcmp(src, "krc1", 4) != 0) {
        fprintf(stderr, "error file format.");
    }

    src += 4;
    for (i = 0; i < st.st_size; i++) {
        src[i] = (unsigned char)(src[i] ^ key[i % 16]);
    }

    decompression(src, st.st_size, result, &dstsize);

    if (argc == 2) {
        char buf[FILENAME_MAX] = {0};
        strncpy(buf, argv[1], strlen(argv[1]) - 4);
        strcat(buf, ".lrc");
        create_lrc(buf, result, dstsize);
    } else if (argc == 3){
        create_lrc(argv[2], result, dstsize);
    }
    
    fclose(fp);
    return 0;
}
