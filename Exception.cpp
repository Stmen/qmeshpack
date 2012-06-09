#include <cstdio>
#include <cstdarg>
#include <cstring>
#include "Exception.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
Exception::Exception(const char* fmt, ...)
{
	if (fmt)
	{
		va_list marker;
		va_start(marker, fmt);
		vsnprintf(_errstr, EXCEPTION_MAX_ERR_STRING_SIZE, fmt, marker);
		va_end(marker);
	}
	else
        strcpy(_errstr, __FUNCTION__);
}
