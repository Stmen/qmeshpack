#include "image.h"
#include <QDebug>
#include <cassert>
#include <cstring>
#include <algorithm>
#include <iostream>
#include "util.h"
#include "veclib/veclib.h"
#include "Exception.h"
using namespace std;

/////////////////////////////////////////////////////////////////////////////////////////////////////
Image::Image(unsigned width, unsigned height, ColorType clearColor) : _width(width), _height(height)
{
	if (width == 0 or _height == 0)
        throw Exception("bad geometry");

    _data = new ColorType[width * height];
    clear(clearColor);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
Image::Image(const Mesh& mesh, Mode mode, unsigned samples_per_pixel)
{
	QVector3D geometry = mesh.getMax() - mesh.getMin();
	if (geometry.x() < 1. or geometry.y() < 1. or geometry.z() < 1.)
		throw Exception("%s: mesh \"%s\" has bad geometry", __FUNCTION__, mesh.getName().toUtf8().constData());

	_width = geometry.x();
	_height = geometry.y();
    _data = new ColorType[_width * _height];

    switch (mode)
    {
		case Top:
			_name = mesh.getName() + "_top";
			clear(0.);
			for (Mesh::Iterator it = mesh.vertexIterator(); it.is_good(); it.next())
			{
				Triangle tri = it.get();
				triangle(tri.vertex[0] - mesh.getMin(), tri.vertex[1] - mesh.getMin(), tri.vertex[2] - mesh.getMin(),
						 Image::maxValue, samples_per_pixel);
			}
			recalcMinMax();
			assert(fabs(_maxColor - geometry.z()) < 1.);
			break;

		case Bottom:
			_name = mesh.getName() + "_bottom";
			clear(geometry.z());
			for (Mesh::Iterator it = mesh.vertexIterator(); it.is_good(); it.next())
			{
				Triangle tri = it.get();
				triangle(tri.vertex[0] - mesh.getMin(), tri.vertex[1] - mesh.getMin(), tri.vertex[2] - mesh.getMin(),
						 Image::minValue, samples_per_pixel);
			}
			recalcMinMax();
			assert(fabs(_minColor) < 1.);
			break;

		default:
			throw Exception("%s: bad drawing mode", __FUNCTION__);
    }

}

/////////////////////////////////////////////////////////////////////////////////////////////////////
Image::~Image()
{
	delete [] _data;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void Image::clear(float value)
{
    for (unsigned i = 0; i < (_width * _height); i++)
        _data[i] = value;

    _minColor = _maxColor = value;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
bool Image::pixelIsInside(float x, float y)
{
    return x >= 0 and x < (float)_width and y >= 0. and y < (float)_height;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void Image::setPixel(unsigned x, unsigned y, ColorType pixel)
{
    _data[y * _width + x] = pixel;
    _minColor = std::min(_minColor, pixel);
    _maxColor = std::max(_maxColor, pixel);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
QImage Image::toQImage() const
{
    assert(_maxColor >= _minColor);
    QImage image(_width, _height, QImage::Format_RGB888);

	float colorStep = (_maxColor - _minColor) / 256.;

    for (unsigned x = 0; x < _width; x++)
    {
        for (unsigned y = 0; y < _height; y++)
        {
			unsigned color = (unsigned)(_data[y * _width + x] * colorStep);
			assert(color < 256);
			unsigned rgbColor =  (color << 16) | (color << 8) | color;
            image.setPixel(x, y, rgbColor);
        }
    }

    return image;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
///
/// creates a border around the image
///
/////////////////////////////////////////////////////////////////////////////////////////////////////
void Image::addBorder(unsigned borderSize, ColorType defaultValue)
{
    if(borderSize == 0)
        throw Exception("wrong border size");

    unsigned newWidth = _width + 2 * borderSize;
    unsigned newHeight = _height + 2 * borderSize;
    float* newData = new float[newWidth * newHeight];

    for (unsigned i = 0; i < (newWidth * newHeight); i++)
        newData[i] = defaultValue;

    for (unsigned y = 0; y < _height; y++)
    {
        memcpy(&newData[y * newWidth + borderSize],
               &_data[y * _width],
               _width * sizeof(float));
    }

    delete[] _data;

    _data = newData;
    _width = newWidth;
    _height = newHeight;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
bool Image::minValue(ColorType imageZ, ColorType newZ)
{
    return imageZ < newZ;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
bool Image::maxValue(ColorType imageZ, ColorType newZ)
{
    return imageZ > newZ;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
///
/// calculates maximum color for a region inside this image.
///
/////////////////////////////////////////////////////////////////////////////////////////////////////
Image::ColorType Image::maxColor(unsigned x, unsigned y, unsigned w, unsigned h) const
{
	ColorType color = at(x, y);
	for (unsigned j = 0; j < h; j++)
	{
		for (unsigned i = 0; i < w; i++)
		{
			color = std::max(color, at(x + i, y + j));
		}
	}
	return color;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
///
/// calculates minimum color for a region inside this image.
///
/////////////////////////////////////////////////////////////////////////////////////////////////////
Image::ColorType Image::minColor(unsigned x, unsigned y, unsigned w, unsigned h) const
{
	ColorType color = at(x, y);
	for (unsigned j = 0; j < h; j++)
	{
		for (unsigned i = 0; i < w; i++)
		{
			color = std::min(color, at(x + i, y + j));
		}
	}
	return color;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void Image::insertAt(unsigned x, unsigned y, unsigned z, const Image &other)
{
	if ((x + other.getWidth() > _width) or (y + other.getHeight() > _height))
		throw Exception("%s: images overlap", __FUNCTION__);

	for (unsigned other_y = 0; other_y < other.getHeight(); other_y++)
	{
		for (unsigned other_x = 0; other_x < other.getWidth(); other_x++)
		{
			ColorType color = other.at(other_x, other_y);
			setPixel(x + other_x, y + other_y, z + color);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
float Image::computeMinZ(unsigned x, unsigned y, const Image &other)
{
	if ((x + other.getWidth() > _width) or (y + other.getHeight() > _height))
		throw Exception("%s: images overlap", __FUNCTION__);

	assert(other.minColor() > -1. and other.minColor() < 1.);

	for (unsigned z = maxColor(x, y, other.getWidth(), other.getHeight()); z > 0; z--)
	{
		for (unsigned j = 0; j < other.getHeight(); j++)
		{
			for (unsigned i = 0; i < other.getWidth(); i++)
			{
				ColorType otherColor = other.at(i, j) + z - 1;
				ColorType color = at(x + i, y + j);
				if (otherColor < color)
					return z;
			}
		}
	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void Image::recalcMinMax()
{
	_minColor = INFINITY;
	_maxColor = -INFINITY;

	for (unsigned i = 0; i < (_width * _height); i++)
	{
		_minColor = std::min(_minColor, _data[i]);
		_maxColor = std::max(_maxColor, _data[i]);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void Image::triangle(QVector3D a, QVector3D b, QVector3D c, bool (&compare)(ColorType, ColorType), unsigned samples_per_pixel)
{
	//QString msg;
	//QDebug(&msg) << a << b << c << "\n";
	//qDebug() << a << b << c;

    // sort by y
    if (a.y() > b.y())
        std::swap(a, b);
    if (a.y() > c.y())
        std::swap(a, c);
    if (b.y() > c.y())
        std::swap(b, c);

    // computing the edge vectors
	QVector3D ab = b - a;
	QVector3D ac = c - a;
	QVector3D bc = c - b;    

	float pixelResolution = 1. / (float) samples_per_pixel;
	if (fabs(ac.y()) < pixelResolution)
        return; // longest edge is too small to make a difference : degenerate triangle, don't draw it

	float t = 0;

	// is AB edge y big enough to draw?
	if (fabs(ab.y()) >= pixelResolution)
    {
        // drawing the spans between AB and AC edges
		for (double y = a.y(); y < b.y(); y += pixelResolution)
        {
			QVector3D start = a + ab * t / ab.y();
			QVector3D end   = a + ac * t / ac.y();

            if (start.x() > end.x())
                std::swap(start, end);

			QVector3D span  = end - start;

			for (double x = start.x(); x < end.x(); x += pixelResolution)
            {
				double progress =  (x - start.x()) / span.x();
				double newValue = start.z() + span.z() * progress;

                if (pixelIsInside(x, y))
                {
                    if (not compare(at(x, y), newValue))
                        setPixel(x, y, newValue);
                }
            }
			t += pixelResolution;
        }
    }

    // is BC edge too small? then ignore it
	if (fabs(bc.y()) >= pixelResolution)
    {
		double d = 0;

		// drawing the spans between AB and AC edges
		for (double y = b.y(); y < c.y(); y += pixelResolution)
        {
			QVector3D start = b + bc * d / bc.y();
			QVector3D end   = a + ac * t / ac.y();

            if (start.x() > end.x())
                std::swap(start, end);

			QVector3D span  = end - start;

			for (double x = start.x(); x < end.x(); x += pixelResolution)
            {
				double progress = (x - start.x()) / span.x();
				double newValue = start.z() + span.z() * progress;

                if (pixelIsInside(x, y))
                {
                    if (not compare(at(x, y), newValue))
                        setPixel(x, y, newValue);
                }
            }
			t += pixelResolution;
			d += pixelResolution;
        }
    }
}
