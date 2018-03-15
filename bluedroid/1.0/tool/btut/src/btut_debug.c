#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <syslog.h>
#include <sys/time.h>
#include <string.h>
#include <time.h>

#include "btut_debug.h"

#define CAM_COLOR_END    "\033[m"
#define CAM_RED          "\033[0;32;31m"
#define CAM_LIGHT_RED    "\033[1;31m"
#define CAM_GREEN        "\033[0;32;32m"
#define CAM_LIGHT_GREEN  "\033[1;32m"
#define CAM_BLUE         "\033[0;32;34m"
#define CAM_LIGHT_BLUE   "\033[1;34m"
#define CAM_DARY_GRAY    "\033[1;30m"
#define CAM_CYAN         "\033[0;36m"
#define CAM_LIGHT_CYAN   "\033[1;36m"
#define CAM_PURPLE       "\033[0;35m"
#define CAM_LIGHT_PURPLE "\033[1;35m"
#define CAM_BROWN        "\033[0;33m"
#define CAM_YELLOW       "\033[1;33m"
#define CAM_LIGHT_GRAY   "\033[0;37m"
#define CAM_WHITE        "\033[1;37m"

typedef struct
{
    unsigned char hour;
    unsigned char min;
    unsigned char sec;
    unsigned short msec;
} _timestamp;

// Default debug level : bit mask
static unsigned char g_btut_log_lv = (BTUT_LOG_LV_VBS | BTUT_LOG_LV_INF | BTUT_LOG_LV_DBG | BTUT_LOG_LV_WRN | BTUT_LOG_LV_ERR);

// Default enable log color
static unsigned char g_btut_log_color = 1;

// Default disable time stamp in log
static unsigned char g_btut_log_timestamp = 0;

// Default tag
static char *g_btut_log_tag = "btcli";


// Output buffer for bt_trace & bt_log_primitive
#define FMT_BUF_SIZE    384

static void _get_timestamp(_timestamp *ts)
{
    time_t rawtime;
    struct tm* timeinfo;
    struct timeval tv;

    if (ts != NULL)
    {
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        gettimeofday(&tv, NULL);

        ts->hour = (unsigned char) timeinfo->tm_hour;
        ts->min = (unsigned char) timeinfo->tm_min;
        ts->sec = (unsigned char) timeinfo->tm_sec;
        ts->msec = (unsigned short) (tv.tv_usec / 1000);
    }
}

static void BTUT_Log_Impl(unsigned char lv, const char *fmt, va_list args, unsigned char color)
{

    char fmt_buf[FMT_BUF_SIZE] = {0};
    char *buf_ptr, *newline, *replace;
    int offset = 0, buf_remain = 0;

    _timestamp ts = {0};
    va_list tmp_args;

    openlog(g_btut_log_tag, 0, 0);
    va_copy(tmp_args, args);
    vsyslog(LOG_DEBUG, fmt, tmp_args);
    closelog();

    if ((lv & g_btut_log_lv))
    {
        buf_ptr = fmt_buf;
        buf_remain = FMT_BUF_SIZE - 1;

        if (g_btut_log_timestamp)
        {
            _get_timestamp(&ts);
            snprintf(buf_ptr, buf_remain, "%02d:%02d:%02d.%03d ", ts.hour, ts.min, ts.sec, ts.msec);

            offset = strlen(fmt_buf);
            buf_ptr += offset;
            buf_remain -= offset;
        }

        if (g_btut_log_color)
        {
            // Apply color prefix.
            switch (lv)
            {
                case BTUT_LOG_LV_ERR:
                    snprintf(buf_ptr, buf_remain, "%s", CAM_LIGHT_RED);
                    break;
                case BTUT_LOG_LV_WRN:
                    snprintf(buf_ptr, buf_remain, "%s", CAM_YELLOW);
                    break;
                case BTUT_LOG_LV_DBG:
                    snprintf(buf_ptr, buf_remain, "%s", CAM_LIGHT_GREEN);
                    break;
                case BTUT_LOG_LV_INF:
                    snprintf(buf_ptr, buf_remain, "%s", CAM_WHITE);
                    break;
                default: // LOG_LV_VBS
                    snprintf(buf_ptr, buf_remain, "%s", CAM_LIGHT_CYAN);
            }

            offset = strlen(fmt_buf);
            buf_ptr = fmt_buf + offset;
            buf_remain -= offset;
        }

        // Apply fmt string.
        snprintf(buf_ptr, buf_remain, "%s", fmt);

        // Replacing newline with space character.
        newline = strchr(fmt_buf, '\n');
        while (newline != NULL)
        {
            replace = newline;
            newline = strchr(newline, '\n');
            *replace = ' ';
        }

        offset = strlen(fmt_buf);
        buf_ptr = fmt_buf + offset;
        buf_remain -= offset;

        if (g_btut_log_color)
        {
            // Apply color postfix
            snprintf(buf_ptr, buf_remain, "%s\n", CAM_COLOR_END);
        }
        else
        {
            snprintf(buf_ptr, buf_remain, "\n");
        }

        va_copy(tmp_args, args);
        vprintf(fmt_buf, tmp_args);
        va_end(tmp_args);
    }

#if 0
    char fmt_buf[FMT_BUF_SIZE] = {0};
    va_list tmp_args;

    openlog("BTCLI", LOG_PID, 0);
    va_copy(tmp_args, args);
    vsyslog(LOG_DEBUG, fmt, tmp_args);
    va_end(tmp_args);
    closelog();



    if ((lv & g_btut_log_lv))
    {
        // Filter log.

        if (color) {
            // Apply color code.
            switch (lv)
            {
                case BTUT_LOG_LV_ERR:
                    snprintf(fmt_buf, FMT_BUF_SIZE, "%s%s%s", CAM_LIGHT_RED, fmt, CAM_COLOR_END);
                    break;
                case BTUT_LOG_LV_WRN:
                    snprintf(fmt_buf, FMT_BUF_SIZE, "%s%s%s", CAM_YELLOW, fmt, CAM_COLOR_END);
                    break;
                case BTUT_LOG_LV_DBG:
                    snprintf(fmt_buf, FMT_BUF_SIZE, "%s%s%s", CAM_LIGHT_GREEN, fmt, CAM_COLOR_END);
                    break;
                case BTUT_LOG_LV_INF:
                    snprintf(fmt_buf, FMT_BUF_SIZE, "%s%s%s", CAM_WHITE, fmt, CAM_COLOR_END);
                    break;
                default: // LOG_LV_VBS
                    snprintf(fmt_buf, FMT_BUF_SIZE, "%s%s%s", CAM_LIGHT_GRAY, fmt, CAM_COLOR_END);
            }
        }
        else
        {
            snprintf(fmt_buf, FMT_BUF_SIZE, "%s\n", fmt);
        }

        va_copy(tmp_args, args);
        vprintf(fmt_buf, tmp_args);
        va_end(tmp_args);
    }
#endif

}

void BTUT_Log(unsigned char lv, const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    BTUT_Log_Impl(lv, fmt, args, 1);
    va_end(args);
}

void BTUT_Log_SetFlag(unsigned short flags)
{
    g_btut_log_lv = flags & BTUT_LOG_MASK;

    g_btut_log_color = (flags & BTUT_LOG_FLAG_COLOR) ? 1 : 0;
    g_btut_log_timestamp= (flags & BTUT_LOG_FLAG_TIMESTAMP) ? 1 : 0;
}

