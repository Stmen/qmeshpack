#include "util.h"
#ifdef ENABLE_TESTS
#include <QDebug>

void test_iterators()
{
	DoubleRangeIterator istart(0, 0, 0, 3, 0, 5);
	DoubleRangeIterator iend = istart.end();

	while(istart != iend)
	{
		qDebug() << "(" << istart._x << ", " << istart._y << ") diff with end : " << (iend - istart) << " value: x: "
				 << ((*istart) & 0xFFFFFFFFU) << " y: " << (((*istart) >> 32) & 0xFFFFFFFFU);
		++istart;
	}
}
#endif
