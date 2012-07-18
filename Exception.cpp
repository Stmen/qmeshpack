#include <QDebug>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include "Exception.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
BaseException::BaseException(QString msg, const char* funcname, const char* filename, unsigned line) :
	_errmsg(msg), _funcname(funcname), _filename(filename), _line(line)
{
	qDebug() << QString("Exception in \'%1\' line %2 in function \'%3\': %4").arg(_filename).arg(_line).arg(_funcname, _errmsg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
BaseException::BaseException(const char* fmt, ...)
{
	if (fmt)
	{
		char str[512];
		va_list marker;
		va_start(marker, fmt);
		vsnprintf(str, 512, fmt, marker);
		va_end(marker);
		_errmsg = QString(str);
	}
	else
		_errmsg = QString("<null>");
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
const char* BaseException::what() const throw ()
{
	return this->_errmsg.toUtf8().constData();
}

