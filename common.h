/*
 * Copyright (C) 2016 Tolga Ceylan
 *
 * CopyPolicy: Released under the terms of the GNU GPL v3.0.
 */
#ifndef __COMMON__H__
#define __COMMON__H__


#define HANDLE_EINTR(x) ({                  \
    decltype(x) _result;                    \
    do                                      \
        _result = (x);                      \
    while (_result == -1 && errno == EINTR);\
    _result;                                \
})

namespace robo {

enum LogLevel
{
    LOG_TRACE,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
};

void logger(LogLevel logLevel, const char *format, ...);


} // namespace robo

#endif // __COMMON__H__