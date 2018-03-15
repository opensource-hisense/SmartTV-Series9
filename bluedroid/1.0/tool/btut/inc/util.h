#ifndef __UTIL_H__
#define __UTIL_H__

#include <hardware/bluetooth.h>

typedef long util_time_t;

typedef struct util_time_t
{
    util_time_t sec;
    util_time_t usec;
} util_time;

#define util_time_before(a, b) \
    ((a)->sec < (b)->sec || \
     ((a)->sec == (b)->sec && (a)->usec < (b)->usec))

#define util_time_sub(a, b, res) do \
{ \
    (res)->sec = (a)->sec - (b)->sec; \
    (res)->usec = (a)->usec - (b)->usec; \
    if ((res)->usec < 0) { \
        (res)->sec--; \
        (res)->usec += 1000000; \
    } \
} while (0)

void util_sleep(util_time_t sec, util_time_t usec);
int util_get_time(util_time *t);
void *util_zalloc(int size);
int util_strlcpy(char *dest, const char *src, int size);
int util_strncasecmp(const char *s1, const char *s2, int n);
char *util_strstr(const char *haystack, const char *needle);
int util_tokenize_cmd(char *cmd, char *argv[], int max_args);
void util_btaddr_stoh(char *btaddr_s, bt_bdaddr_t *bdaddr_h);
#endif
