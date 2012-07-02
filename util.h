#pragma once
#include <QString>
#include <QVector3D>
#include <cmath>
#include "config.h"

union vec3i
{
	typedef int v4si __attribute__ ((vector_size (16)));

	v4si data;
	int	 elem[4];

	inline int x() const { return elem[0]; }
	inline int y() const { return elem[1]; }
	inline int z() const { return elem[2]; }

	inline vec3i operator+(const vec3i other) { return { data + other.data }; }
	inline vec3i operator-(const vec3i other) { return { data - other.data }; }
	inline vec3i operator*(const vec3i other) { return { data * other.data }; }
	inline vec3i operator/(const vec3i other) { return { data / other.data }; }
	inline vec3i operator*(const int i)
	{
		v4si v =  {i, i, i, 0};
		return { data * v };
	}

	inline vec3i operator/(const int i)
	{
		v4si v =  {i, i, i, 1};
		return { data / v };
	}

	static vec3i from(int x, int y, int z)
	{
		v4si result = {x, y, z, 0};
		return { result };
	}

	static vec3i from(QVector3D v)
	{
		//v += QVector3D(0.5, 0.5, 0.5); // rounding
		v4si result = {(int)v.x(), (int)v.y(), (int)v.z(), 0};
		return { result };
	}

};

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


#include <iterator>
#include <cassert>
struct DoubleRangeIterator : public  std::iterator<std::bidirectional_iterator_tag, quint64>
{
	quint64 _x;
	quint64 _y;
	quint32 _min_x, _max_x, _min_y, _max_y;

	DoubleRangeIterator(quint32 min_x, quint32 max_x, quint32 min_y, quint32 max_y) :
		_x(min_x), _y(min_y), _min_x(min_x), _max_x(max_x), _min_y(min_y), _max_y(max_y)
	{
	}

	DoubleRangeIterator(quint32 x, quint32 y, quint32 min_x, quint32 max_x, quint32 min_y, quint32 max_y) :
		_x(x), _y(y), _min_x(min_x), _max_x(max_x), _min_y(min_y), _max_y(max_y)
	{
		assert(x >= _min_x and x < _max_x);
		assert(y >= _min_y and y <= _max_y);
	}

	DoubleRangeIterator(const DoubleRangeIterator& other, quint32 x, quint32 y) :
		_x(x), _y(y), _min_x(other._min_x), _max_x(other._max_x), _min_y(other._min_y), _max_y(other._max_y)
	{
		assert(x >= _min_x and x < _max_x);
		assert(y >= _min_y and y <= _max_y);
	}

	DoubleRangeIterator(const DoubleRangeIterator& other)
	{
		(*this) = other;
	}

	DoubleRangeIterator& operator = (const DoubleRangeIterator& other)
	{
		memcpy(this, &other, sizeof(DoubleRangeIterator));
		return *this;
	}

	quint64 operator * () const
	{
		return ((quint64)_y << 32) | ((quint64)_x & 0xFFFFFFFF);
	}

	bool operator == (const DoubleRangeIterator& other) const
	{
		return (_x == other._x) and (_y == other._y);
	}

	bool operator != (const DoubleRangeIterator& other) const
	{
		return (_x != other._x) or (_y != other._y);
	}

	void operator ++ ()
	{
		if (_y != _max_y)
		{
			_x++;
			if (_x == _max_x)
			{
				_x = _min_x;
				_y++;
			}
		}
	}



	/// Advances the iterator by n items
	void operator += (ptrdiff_t n)
	{
		if (n < 0)
			(*this)-= -n;

		for (unsigned i = 0; i < n; ++i)
			++(*this);
	}

	/// --i	Moves the iterator back by one item
	void operator -- ()
	{
		if (_x == _min_x)
		{
			_x = _max_x - 1;
			if (_y == _min_y)
			{
				_y = _max_y - 1;
			}
			else
				_y--;
		}
		else
			_x--;
	}

	//i -= n	Moves the iterator back by n items
	void operator -= (ptrdiff_t n)
	{
		if (n < 0)
			(*this)+= -n;

		for (unsigned i = 0; i < n; ++i)
			--(*this);
	}

	DoubleRangeIterator end() const
	{
		return DoubleRangeIterator(*this, _min_x, _max_y);

	}

	bool typeEqual(const DoubleRangeIterator& other) const
	{
		return _min_x == other._min_x and _min_y == other._min_y and _max_x == other._max_x and _max_y == other._max_y;
	}

	bool operator < (const DoubleRangeIterator& other) const
	{
		return (_y < other._y or (_y == other._y and _x < other._x));
	}

	// i - j	Returns the number of items between iterators i and j
	qint64 operator-(const DoubleRangeIterator& other) const
	{
		assert(typeEqual(other));
		quint32 range_x = _max_x - _min_x;

		if (*this < other)
		{
			return -(other - (*this));
		}

		return (qint64)((_y - other._y) * range_x + (_x - other._x));
	}

};

#ifdef ENABLE_TESTS
void test_iterators();
#endif

