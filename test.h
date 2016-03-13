
#ifndef __TEST__H__
#define __TEST__H__

#include <stdint.h>
#include <stdlib.h>
#include <cstdarg>

enum LogLevel
{
	LOG_TRACE,
	LOG_DEBUG,
	LOG_INFO,
	LOG_WARN,
	LOG_ERROR,
};

static inline const char *get_log_level_str(LogLevel logLevel)
{
	switch (logLevel)
	{
		case LOG_TRACE: return "TRACE ";
		case LOG_DEBUG: return "DEBUG ";
		case LOG_INFO: 	return "INFO  ";
		case LOG_WARN: 	return "WARN  ";
		case LOG_ERROR:	return "ERROR ";
		default: break;
	}
	return "????? ";
}

static inline void logger(LogLevel logLevel, const char *format, ...)
{
	printf("%s", get_log_level_str(logLevel));

	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);

	printf("\n");
}

#define HANDLE_EINTR(x) ({                   \
  decltype(x) _result;                         \
  do                                         \
    _result = (x);                           \
  while (_result == -1 && errno == EINTR);   \
  _result;                                   \
})


#endif // __TEST__H__