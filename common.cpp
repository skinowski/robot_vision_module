/*
 * Copyright (C) 2016 Tolga Ceylan
 *
 * CopyPolicy: Released under the terms of the GNU GPL v3.0.
 */
#include "common.h"

#include <stdarg.h>
#include <stdio.h>

namespace robo {

static inline const char *get_log_level_str(LogLevel logLevel)
{
    switch (logLevel)
    {
        case LOG_TRACE: return "TRACE ";
        case LOG_DEBUG: return "DEBUG ";
        case LOG_INFO:  return "INFO  ";
        case LOG_WARN:  return "WARN  ";
        case LOG_ERROR: return "ERROR ";
        default: break;
    }
    return "????? ";
}

void logger(LogLevel logLevel, const char *format, ...)
{
    printf("%s", get_log_level_str(logLevel));

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    printf("\n");
}

} // namespace robo