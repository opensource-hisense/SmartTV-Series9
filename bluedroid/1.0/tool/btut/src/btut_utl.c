#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "util.h"

int util_get_time(util_time *t)
{
    int res;
    struct timeval tv;
    res = gettimeofday(&tv, NULL);
    t->sec = tv.tv_sec;
    t->usec = tv.tv_usec;
    return res;
}

void *util_zalloc(int size)
{
    void *ptr = malloc(size);
    if (ptr)
        memset(ptr, 0, (size_t) size);
    return ptr;
}

int util_strlcpy(char *dest, const char *src, int size)
{
    const char *s = src;
    int left = size;

    if (left)
    {
        /* Copy string up to the maximum size of the dest buffer */
        while (--left != 0)
        {
            if ((*dest++ = *s++) == '\0')
                break;
        }
    }

    if (left == 0)
    {
        /* Not enough room for the string; force NUL-termination */
        if (size != 0)
            *dest = '\0';
        while (*s++)
            ; /* determine total src string length */
    }

    return s - src - 1;
}


int util_tokenize_cmd(char *cmd, char *argv[], int max_args)
{
    char *pos, *pos2;
    int argc = 0;

    pos = cmd;
    while (1)
    {
        // Trim the space characters.
        while (*pos == ' ')
        {
            pos++;
        }

        if (*pos == '\0')
        {
            break;
        }

        // One token may start with '"' or other characters.
        if (*pos == '"' && (pos2 = strchr(pos, '"')))
        {
            argv[argc++] = pos + 1;
            *pos2 = '\0';
            pos = pos2 + 1;

        }
        else
        {
            argv[argc++] = pos;
            while (*pos != '\0' && *pos != ' ')
            {
                pos++;
            }
            *pos++ = '\0';
        }

        // Check if the maximum of arguments is reached.
        if (argc == max_args)
        {
            break;
        }
    }

    return argc;
}

void util_btaddr_stoh(char *btaddr_s, bt_bdaddr_t *bdaddr_h)
{
    int i;
    for (i = 0; i <6; i++)
    {
        bdaddr_h->address[i] = strtoul(btaddr_s, &btaddr_s, 16);
        btaddr_s++;
    }
}
