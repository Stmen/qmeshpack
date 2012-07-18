#pragma once
#include <QtConcurrentMap>
#include <QString>

class BaseException: public QtConcurrent::Exception
{	
	public:
	BaseException() {}
	BaseException(const char* fmt, ...);
	BaseException(QString msg, const char* funcname, const char* filename, unsigned line);

	virtual ~BaseException() throw ()
	{
	}

	virtual void raise() const
	{
		throw *this;
	}

	virtual BaseException* clone() const
	{
		return new BaseException(*this);
	}

	const char* what() const throw ();

protected:

	QString _errmsg;
	const char*	_funcname;
	const char*	_filename;
	unsigned	_line;
};

struct ImageException : public BaseException
{
	ImageException(QString msg, const char* funcname, const char* filename, unsigned line) : BaseException(msg,funcname, filename, line) {}

	void raise() const
	{
		throw *this;
	}

	ImageException* clone() const
	{
		return new ImageException(*this);
	}

};

struct MeshException : public BaseException
{
	MeshException(QString msg, const char* funcname, const char* filename, unsigned line) : BaseException(msg,funcname, filename, line) {}

	void raise() const
	{
		throw *this;
	}

	MeshException* clone() const
	{
		return new MeshException(*this);
	}
};

#ifdef __GNUC__
#define THROW(ex, S) throw ex((S), __PRETTY_FUNCTION__, __FILE__, __LINE__)
#else
#define THROW(ex, S) throw ex((S), __FUNCTION__, __FILE__, __LINE__)
#endif
