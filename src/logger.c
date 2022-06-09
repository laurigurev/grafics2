#include "grafics2.h"

static const char* level_str[] = 
{
	"INFO ", "DEBUG", "TRACE", "WARN ", "ERROR"
};

typedef struct
{
	FILE* f;
	bool is_initd;
	bool is_closed;
} logger;

static logger LOGGER = (logger) { NULL, false, false };

void log_init(const char* file)
{
	if (!LOGGER.is_initd)
	{
		LOGGER.f = fopen(file, "w");
		LOGGER.is_initd = true;
		LOGGER.is_closed = false;
	}
}

void glog(int level, const char* file, int line, const char* format, ...)
{	
	// h:min:sec LEVEL file.c:line: format
	time_t now = time(NULL);
	struct tm* tm_struct = localtime(&now);
	
	va_list args;
    va_start(args, format);
	fprintf(
	   	LOGGER.f, "%.2i:%.2i:%.2i %s %s:%i: ",
		(int) tm_struct->tm_hour, (int) tm_struct->tm_min,
   		(int) tm_struct->tm_sec, level_str[level], file, line
	);
	vfprintf(LOGGER.f, format, args);
	va_end(args);
}

void flushl()
{
	fflush(LOGGER.f);
}

void log_close()
{
	// causes SIGTRAP in gdb
	if (!LOGGER.is_closed)
	{
		fclose(LOGGER.f);
		LOGGER.is_initd = false;
		LOGGER.is_closed = true;
	}
}
