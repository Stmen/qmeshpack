#pragma once

#include <iostream>
#define EXCEPTION_MAX_ERR_STRING_SIZE 512

class Exception: public std::exception
{
public:
	char _errstr[EXCEPTION_MAX_ERR_STRING_SIZE];

	Exception(const char* fmt, ...);
	virtual ~Exception() throw ()
	{
	}

    const char* what() const throw () override
	{
		return _errstr;
	}
};

