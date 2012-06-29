#include <cstdio>
#include <cstdarg>
#include <cstring>
#include "Exception.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
/*
GeneralException::GeneralException(const char* fmt, ...)
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
*/

#include <QDebug>

//////////////////////////////////////////////////////////////////////////////////////////////////////
BaseException::BaseException(QString msg, const char* funcname, const char* filename, unsigned line) :
	_errmsg(msg), _funcname(funcname), _filename(filename), _line(line)
{
	qDebug() << QString("Exception in \'%1\' line %2 in function \'%3\': %4").arg(_filename).arg(_line).arg(_funcname, _errmsg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
const char* BaseException::what() const throw ()
{
	return this->_errmsg.toUtf8().constData();
}
