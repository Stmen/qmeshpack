#pragma once
#include <QString>
#include <QVector3D>
#include <cmath>

template<class T>
inline T lerp(T a, T b, float percent)
{
    return (b - a) * percent;
}

inline QVector3D ceil(QVector3D v)
{
	return QVector3D(::ceil(v.x()), ::ceil(v.y()), ::ceil(v.z()));
}

inline QVector3D floor(QVector3D v)
{
	return QVector3D(::floor(v.x()), ::floor(v.y()), ::floor(v.z()));
}

inline QVector3D vecmax(QVector3D v1, QVector3D v2)
{
	return QVector3D(((v1.x() > v2.x()) ? v1.x() : v2.x()),
					 ((v1.y() > v2.y()) ? v1.y() : v2.y()),
					 ((v1.z() > v2.z()) ? v1.z() : v2.z()));
}

inline QVector3D vecmin(QVector3D v1, QVector3D v2)
{
	return QVector3D(((v1.x() < v2.x()) ? v1.x() : v2.x()),
					 ((v1.y() < v2.y()) ? v1.y() : v2.y()),
					 ((v1.z() < v2.z()) ? v1.z() : v2.z()));
}

inline QString toString(QVector3D vec)
{
	return QString("(%1, %2, %3)").arg(vec.x()).arg(vec.y()).arg(vec.z());
}

template<class T>
class Iterator
{
public:
    inline virtual ~Iterator() {}
    virtual bool is_good() const = 0;
    virtual T get() const = 0;
    virtual void next() = 0;
};

/// array smart pointer
template<class T>
struct array_ptr
{
    T* _data;

	inline array_ptr(T* d = 0) : _data(d) {}
	inline ~array_ptr() { reset(); }
	inline void reset() { if(_data) delete[] _data; }
	inline void release() { _data = 0; }
	inline T* get() { return _data; }

private:

	array_ptr(const array_ptr& other);
	array_ptr& operator=(const array_ptr& other);
};
