/*
 * This file is part of the EasyLogger Library.
 *
 * Copyright (c) 2015-2016, Armink, <armink.ztl@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Function: Initialize function and other general function.
 * Created on: 2015-04-28
 */

#include <elog.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#if !defined(ELOG_OUTPUT_LVL)
    #error "Please configure static output log level (in elog_cfg.h)"
#endif

#if !defined(ELOG_LINE_NUM_MAX_LEN)
    #error "Please configure output line number max length (in elog_cfg.h)"
#endif

#if !defined(ELOG_LINE_BUF_SIZE)
    #error "Please configure buffer size for every line's log (in elog_cfg.h)"
#endif

#if !defined(ELOG_FILTER_TAG_MAX_LEN)
    #error "Please configure output filter's tag max length (in elog_cfg.h)"
#endif

#if !defined(ELOG_FILTER_KW_MAX_LEN)
    #error "Please configure output filter's keyword max length (in elog_cfg.h)"
#endif

#if !defined(ELOG_NEWLINE_SIGN)
    #error "Please configure output newline sign (in elog_cfg.h)"
#endif

/**
 * CSI(Control Sequence Introducer/Initiator) sign
 * more information on https://en.wikipedia.org/wiki/ANSI_escape_code
 */
#define CSI_START                                   "\033["
#define CSI_END                                   "\033[0m"
/* output log front color */
#define F_BLACK                                       "30;"
#define F_RED                                         "31;"
#define F_GREEN                                       "32;"
#define F_YELLOW                                      "33;"
#define F_BLUE                                        "34;"
#define F_MAGENTA                                     "35;"
#define F_CYAN                                        "36;"
#define F_WHITE                                       "37;"
/* output log background color */
#define B_NULL
#define B_BLACK                                       "40;"
#define B_RED                                         "41;"
#define B_GREEN                                       "42;"
#define B_YELLOW                                      "43;"
#define B_BLUE                                        "44;"
#define B_MAGENTA                                     "45;"
#define B_CYAN                                        "46;"
#define B_WHITE                                       "47;"
/* output log fonts style */
#define S_BOLD                                         "1m"
#define S_UNDERLINE                                    "4m"
#define S_BLINK                                        "5m"
#define S_NORMAL                                      "22m"
/* output log default color definition: [front color] + [background color] + [show style] */
#ifndef ELOG_COLOR_ASSERT
#define ELOG_COLOR_ASSERT       (F_MAGENTA B_NULL S_NORMAL)
#endif
#ifndef ELOG_COLOR_ERROR
#define ELOG_COLOR_ERROR            (F_RED B_NULL S_NORMAL)
#endif
#ifndef ELOG_COLOR_WARN
#define ELOG_COLOR_WARN          (F_YELLOW B_NULL S_NORMAL)
#endif
#ifndef ELOG_COLOR_INFO
#define ELOG_COLOR_INFO            (F_CYAN B_NULL S_NORMAL)
#endif
#ifndef ELOG_COLOR_DEBUG
#define ELOG_COLOR_DEBUG          (F_GREEN B_NULL S_NORMAL)
#endif
#ifndef ELOG_COLOR_VERBOSE
#define ELOG_COLOR_VERBOSE         (F_BLUE B_NULL S_NORMAL)
#endif

/* EasyLogger object */
static EasyLogger elog;
/* every line log's buffer */
static char log_buf[ELOG_LINE_BUF_SIZE] = { 0 };
/* log tag */
static const char *log_tag = "elog";
/* level output info */
static const char *level_output_info[] = {
        [ELOG_LVL_ASSERT]  = "A/",
        [ELOG_LVL_ERROR]   = "E/",
        [ELOG_LVL_WARN]    = "W/",
        [ELOG_LVL_INFO]    = "I/",
        [ELOG_LVL_DEBUG]   = "D/",
        [ELOG_LVL_VERBOSE] = "V/",
};
/* color output info */
static const char *color_output_info[] = {
        [ELOG_LVL_ASSERT]  = ELOG_COLOR_ASSERT,
        [ELOG_LVL_ERROR]   = ELOG_COLOR_ERROR,
        [ELOG_LVL_WARN]    = ELOG_COLOR_WARN,
        [ELOG_LVL_INFO]    = ELOG_COLOR_INFO,
        [ELOG_LVL_DEBUG]   = ELOG_COLOR_DEBUG,
        [ELOG_LVL_VERBOSE] = ELOG_COLOR_VERBOSE,
};

static bool get_fmt_enabled(uint8_t level, size_t set);

/* EasyLogger assert hook */
void (*elog_assert_hook)(const char* expr, const char* func, size_t line);

extern void elog_port_output(const char *log, size_t size);
extern void elog_port_output_lock(void);
extern void elog_port_output_unlock(void);

/**
 * EasyLogger initialize.
 *
 * @return result
 */
ElogErrCode elog_init(void) {
    extern ElogErrCode elog_port_init(void);
    extern ElogErrCode elog_async_init(void);

    ElogErrCode result = ELOG_NO_ERR;

    /* port initialize */
    result = elog_port_init();
    if (result != ELOG_NO_ERR) {
        return result;
    }

#ifdef ELOG_ASYNC_OUTPUT_ENABLE
    result = elog_async_init();
    if (result != ELOG_NO_ERR) {
        return result;
    }
#endif

    /* enable the output lock */
    elog_output_lock_enabled(true);
    /* output locked status initialize */
    elog.output_is_locked_before_enable = false;
    elog.output_is_locked_before_disable = false;
    /* disable text color by default */
    elog_set_text_color_enabled(false);
    /* set level is ELOG_LVL_VERBOSE */
    elog_set_filter_lvl(ELOG_LVL_VERBOSE);

    return result;
}

/**
 * EasyLogger start after initialize.
 */
void elog_start(void) {
    /* enable output */
    elog_set_output_enabled(true);

#if defined(ELOG_ASYNC_OUTPUT_ENABLE)
    elog_async_enabled(true);
#elif defined(ELOG_BUF_OUTPUT_ENABLE)
    elog_buf_enabled(true);
#endif

    /* show version */
    elog_i(log_tag, "EasyLogger V%s is initialize success.", ELOG_SW_VERSION);
    elog_i(log_tag, "You can get the latest version on https://github.com/armink/EasyLogger .");
}

/**
 * set output enable or disable
 *
 * @param enabled TRUE: enable FALSE: disable
 */
void elog_set_output_enabled(bool enabled) {
    ELOG_ASSERT((enabled == false) || (enabled == true));

    elog.output_enabled = enabled;
}

/**
 * set log text color enable or disable
 * 
 * @param enabled TRUE: enable FALSE:disable
 */
void elog_set_text_color_enabled(bool enabled) {
    elog.text_color_enabled = enabled;
}

/**
 * get log text color enable status
 *
 * @return enable or disable
 */
bool elog_get_text_color_enabled(void) {
    return elog.text_color_enabled;
}

/**
 * get output is enable or disable
 *
 * @return enable or disable
 */
bool elog_get_output_enabled(void) {
    return elog.output_enabled;
}

/**
 * set log output format. only enable or disable
 *
 * @param level level
 * @param set format set
 */
void elog_set_fmt(uint8_t level, size_t set) {
    ELOG_ASSERT(level <= ELOG_LVL_VERBOSE);

    elog.enabled_fmt_set[level] = set;
}

/**
 * set log filter all parameter
 *
 * @param level level
 * @param tag tag
 * @param keyword keyword
 */
void elog_set_filter(uint8_t level, const char *tag, const char *keyword) {
    ELOG_ASSERT(level <= ELOG_LVL_VERBOSE);

    elog_set_filter_lvl(level);
    elog_set_filter_tag(tag);
    elog_set_filter_kw(keyword);
}

/**
 * set log filter's level
 *
 * @param level level
 */
void elog_set_filter_lvl(uint8_t level) {
    ELOG_ASSERT(level <= ELOG_LVL_VERBOSE);

    elog.filter.level = level;
}

/**
 * set log filter's tag
 *
 * @param tag tag
 */
void elog_set_filter_tag(const char *tag) {
    strncpy(elog.filter.tag, tag, ELOG_FILTER_TAG_MAX_LEN);
}

/**
 * set log filter's keyword
 *
 * @param keyword keyword
 */
void elog_set_filter_kw(const char *keyword) {
    strncpy(elog.filter.keyword, keyword, ELOG_FILTER_KW_MAX_LEN);
}

/**
 * lock output
 */
void elog_output_lock(void) {
    if (elog.output_lock_enabled) {
        elog_port_output_lock();
        elog.output_is_locked_before_disable = true;
    } else {
        elog.output_is_locked_before_enable = true;
    }
}

/**
 * unlock output
 */
void elog_output_unlock(void) {
    if (elog.output_lock_enabled) {
        elog_port_output_unlock();
        elog.output_is_locked_before_disable = false;
    } else {
        elog.output_is_locked_before_enable = false;
    }
}

/**
 * output RAW format log
 *
 * @param format output format
 * @param ... args
 */
void elog_raw(const char *format, ...) {
    va_list args;
    size_t log_len = 0;
    int fmt_result;

    /* check output enabled */
    if (!elog.output_enabled) {
        return;
    }

    /* args point to the first variable parameter */
    va_start(args, format);

    /* lock output */
    elog_output_lock();

    /* package log data to buffer */
    fmt_result = vsnprintf(log_buf, ELOG_LINE_BUF_SIZE, format, args);

    /* output converted log */
    if ((fmt_result > -1) && (fmt_result <= ELOG_LINE_BUF_SIZE)) {
        log_len = fmt_result;
    } else {
        log_len = ELOG_LINE_BUF_SIZE;
    }
    /* output log */
#if defined(ELOG_ASYNC_OUTPUT_ENABLE)
    extern void elog_async_output(const char *log, size_t size);
    elog_async_output(log_buf, log_len);
#elif defined(ELOG_BUF_OUTPUT_ENABLE)
    extern void elog_buf_output(const char *log, size_t size);
    elog_buf_output(log_buf, log_len);
#else
    elog_port_output(log_buf, log_len);
#endif
    /* unlock output */
    elog_port_output_unlock();

    va_end(args);
}

/**
 * output the log
 *
 * @param level level
 * @param tag tag
 * @param file file name
 * @param func function name
 * @param line line number
 * @param format output format
 * @param ... args
 *
 */
void elog_output(uint8_t level, const char *tag, const char *file, const char *func,
        const long line, const char *format, ...) {
    extern const char *elog_port_get_time(void);
    extern const char *elog_port_get_p_info(void);
    extern const char *elog_port_get_t_info(void);

    size_t tag_len = strlen(tag), log_len = 0, newline_len = strlen(ELOG_NEWLINE_SIGN);
    char line_num[ELOG_LINE_NUM_MAX_LEN + 1] = { 0 };
    char tag_sapce[ELOG_FILTER_TAG_MAX_LEN / 2 + 1] = { 0 };
    va_list args;
    int fmt_result;

    ELOG_ASSERT(level <= ELOG_LVL_VERBOSE);

    /* check output enabled */
    if (!elog.output_enabled) {
        return;
    }
    /* level filter */
    if (level > elog.filter.level) {
        return;
    } else if (!strstr(tag, elog.filter.tag)) { /* tag filter */
        //TODO 可以考虑采用KMP及朴素模式匹配字符串，提升性能
        return;
    }
    /* args point to the first variable parameter */
    va_start(args, format);
    /* lock output */
    elog_output_lock();
    /* add CSI start sign and color info */
    if (elog.text_color_enabled) {
        log_len += elog_strcpy(log_len, log_buf + log_len, CSI_START);
        log_len += elog_strcpy(log_len, log_buf + log_len, color_output_info[level]);
    }
    /* package level info */
    if (get_fmt_enabled(level, ELOG_FMT_LVL)) {
        log_len += elog_strcpy(log_len, log_buf + log_len, level_output_info[level]);
    }
    /* package tag info */
    if (get_fmt_enabled(level, ELOG_FMT_TAG)) {
        log_len += elog_strcpy(log_len, log_buf + log_len, tag);
        /* if the tag length is less than 50% ELOG_FILTER_TAG_MAX_LEN, then fill space */
        if (tag_len <= ELOG_FILTER_TAG_MAX_LEN / 2) {
            memset(tag_sapce, ' ', ELOG_FILTER_TAG_MAX_LEN / 2 - tag_len);
            log_len += elog_strcpy(log_len, log_buf + log_len, tag_sapce);
        }
        log_len += elog_strcpy(log_len, log_buf + log_len, " ");
    }
    /* package time, process and thread info */
    if (get_fmt_enabled(level, ELOG_FMT_TIME | ELOG_FMT_P_INFO | ELOG_FMT_T_INFO)) {
        log_len += elog_strcpy(log_len, log_buf + log_len, "[");
        /* package time info */
        if (get_fmt_enabled(level, ELOG_FMT_TIME)) {
            log_len += elog_strcpy(log_len, log_buf + log_len, elog_port_get_time());
            if (get_fmt_enabled(level, ELOG_FMT_P_INFO | ELOG_FMT_T_INFO)) {
                log_len += elog_strcpy(log_len, log_buf + log_len, " ");
            }
        }
        /* package process info */
        if (get_fmt_enabled(level, ELOG_FMT_P_INFO)) {
            log_len += elog_strcpy(log_len, log_buf + log_len, elog_port_get_p_info());
            if (get_fmt_enabled(level, ELOG_FMT_T_INFO)) {
                log_len += elog_strcpy(log_len, log_buf + log_len, " ");
            }
        }
        /* package thread info */
        if (get_fmt_enabled(level, ELOG_FMT_T_INFO)) {
            log_len += elog_strcpy(log_len, log_buf + log_len, elog_port_get_t_info());
        }
        log_len += elog_strcpy(log_len, log_buf + log_len, "] ");
    }
    /* package file directory and name, function name and line number info */
    if (get_fmt_enabled(level, ELOG_FMT_DIR | ELOG_FMT_FUNC | ELOG_FMT_LINE)) {
        log_len += elog_strcpy(log_len, log_buf + log_len, "(");
        /* package time info */
        if (get_fmt_enabled(level, ELOG_FMT_DIR)) {
            log_len += elog_strcpy(log_len, log_buf + log_len, file);
            if (get_fmt_enabled(level, ELOG_FMT_FUNC)) {
                log_len += elog_strcpy(log_len, log_buf + log_len, " ");
            } else if (get_fmt_enabled(level, ELOG_FMT_LINE)) {
                log_len += elog_strcpy(log_len, log_buf + log_len, ":");
            }
        }
        /* package process info */
        if (get_fmt_enabled(level, ELOG_FMT_FUNC)) {
            log_len += elog_strcpy(log_len, log_buf + log_len, func);
            if (get_fmt_enabled(level, ELOG_FMT_LINE)) {
                log_len += elog_strcpy(log_len, log_buf + log_len, ":");
            }
        }
        /* package thread info */
        if (get_fmt_enabled(level, ELOG_FMT_LINE)) {
            //TODO snprintf资源占用可能较高，待优化
            snprintf(line_num, ELOG_LINE_NUM_MAX_LEN, "%ld", line);
            log_len += elog_strcpy(log_len, log_buf + log_len, line_num);
        }
        log_len += elog_strcpy(log_len, log_buf + log_len, ")");
    }
    /* package other log data to buffer. '\0' must be added in the end by vsnprintf. */
    fmt_result = vsnprintf(log_buf + log_len, ELOG_LINE_BUF_SIZE - log_len - newline_len + 1, format, args);

    va_end(args);
    /* add CSI end sign */
    if (elog.text_color_enabled) {
        log_len += elog_strcpy(log_len, log_buf + log_len + fmt_result, CSI_END);
    }
    /* keyword filter */
    if (!strstr(log_buf, elog.filter.keyword)) {
        //TODO 可以考虑采用KMP及朴素模式匹配字符串，提升性能
        /* unlock output */
        elog_output_unlock();
        return;
    }
    /* package newline sign */
    if ((fmt_result > -1) && (fmt_result + log_len + newline_len <= ELOG_LINE_BUF_SIZE)) {
        log_len += fmt_result;
        log_len += elog_strcpy(log_len, log_buf + log_len, ELOG_NEWLINE_SIGN);
    } else {
        log_len = ELOG_LINE_BUF_SIZE;
        /* copy newline sign */
        strcpy(log_buf + ELOG_LINE_BUF_SIZE - newline_len, ELOG_NEWLINE_SIGN);
    }
    /* output log */
#if defined(ELOG_ASYNC_OUTPUT_ENABLE)
    extern void elog_async_output(const char *log, size_t size);
    elog_async_output(log_buf, log_len);
#elif defined(ELOG_BUF_OUTPUT_ENABLE)
    extern void elog_buf_output(const char *log, size_t size);
    elog_buf_output(log_buf, log_len);
#else
    elog_port_output(log_buf, log_len);
#endif
    /* unlock output */
    elog_output_unlock();
}

/**
 * get format enabled
 *
 * @param level level
 * @param set format set
 *
 * @return enable or disable
 */
static bool get_fmt_enabled(uint8_t level, size_t set) {
    ELOG_ASSERT(level <= ELOG_LVL_VERBOSE);

    if (elog.enabled_fmt_set[level] & set) {
        return true;
    } else {
        return false;
    }
}

/**
 * enable or disable logger output lock
 * @note disable this lock is not recommended except you want output system exception log
 *
 * @param enabled true: enable  false: disable
 */
void elog_output_lock_enabled(bool enabled) {
    elog.output_lock_enabled = enabled;
    /* it will re-lock or re-unlock before output lock enable */
    if (elog.output_lock_enabled) {
        if (!elog.output_is_locked_before_disable && elog.output_is_locked_before_enable) {
            /* the output lock is unlocked before disable, and the lock will unlocking after enable */
            elog_port_output_lock();
        } else if (elog.output_is_locked_before_disable && !elog.output_is_locked_before_enable) {
            /* the output lock is locked before disable, and the lock will locking after enable */
            elog_port_output_unlock();
        }
    }
}

/**
 * Set a hook function to EasyLogger assert. It will run when the expression is false.
 *
 * @param hook the hook function
 */
void elog_assert_set_hook(void (*hook)(const char* expr, const char* func, size_t line)) {
    elog_assert_hook = hook;
}
