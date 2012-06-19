#include "image.h"
#include <QDebug>
#include <cassert>
#include <cstring>
#include <algorithm>
#include <iostream>
#include "util.h"
#include "Exception.h"
using namespace std;

/////////////////////////////////////////////////////////////////////////////////////////////////////
Image::Image(unsigned width, unsigned height, ColorType clearColor) :
	_width(width),
	_height(height)
{
	if (width == 0 or _height == 0)
        throw Exception("bad geometry");

    _data = new ColorType[width * height];
    clear(clearColor);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
Image::Image(const Mesh& mesh, Mode mode, unsigned dilationValue)
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
						 Image::maxValue);
			}			
			assert(fabs(_maxColor - geometry.z()) < 1.);
			dilate(dilationValue, Image::maxValue);			
			break;

		case Bottom:
			_name = mesh.getName() + "_bottom";
			clear(geometry.z());
			for (Mesh::Iterator it = mesh.vertexIterator(); it.is_good(); it.next())
			{
				Triangle tri = it.get();
				triangle(tri.vertex[0] - mesh.getMin(), tri.vertex[1] - mesh.getMin(), tri.vertex[2] - mesh.getMin(),
						 Image::minValue);
			}
			assert(fabs(_minColor) < 1.);
			dilate(dilationValue, Image::minValue);
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
bool Image::pixelIsInside(int x, int y)
{
	return x >= 0 and x < (float)_width and y >= 0 and y < (float)_height;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void Image::setPixel(unsigned x, unsigned y, ColorType pixel)
{
	assert(x < _width);
	assert(y < _height);

    _data[y * _width + x] = pixel;
    _minColor = std::min(_minColor, pixel);
    _maxColor = std::max(_maxColor, pixel);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
QImage Image::toQImage() const
{	
	assert(_maxColor >= _minColor);
    QImage image(_width, _height, QImage::Format_RGB888);

	float colorStep = 0;
	if(_maxColor != _minColor)
		colorStep = 255. / (_maxColor - _minColor);

    for (unsigned x = 0; x < _width; x++)
    {
        for (unsigned y = 0; y < _height; y++)
        {
			ColorType colorValue = _data[y * _width + x];
			int color = ((colorValue - _minColor) * colorStep);
			assert(color >= 0 and color < 256);
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
float Image::computeMinZ(unsigned x, unsigned y, const Image &other) const
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
bool circlePredicate(unsigned x, unsigned y, unsigned radius2)
{
	return (x * x + y * y) < radius2;
	//return true;

}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void Image::dilate(int dilationValue, bool (&compare)(ColorType, ColorType))
{
	if (dilationValue == 0)
		return;

	ColorType clearColor;
	if (compare(_maxColor, _minColor))
		clearColor = _minColor;
	else if (compare(_minColor, _maxColor))
		clearColor = _maxColor;
	else
		throw Exception("bad compare func");

	Image newImage(dilationValue * 2 + _width, dilationValue * 2 + _height, clearColor);
	unsigned dilation2 = dilationValue * dilationValue;

	for (unsigned x = 0; x < _width; x++)
	{
		for (unsigned y = 0; y < _height; y++)
		{
			ColorType color = at(x, y);
			if (color != clearColor)
			{
				ColorType newColor = ((clearColor == _minColor) ? (color + dilationValue) : (color - dilationValue));

				for (unsigned j = x - dilationValue; j < (x + dilationValue); j++)
				{
					for (unsigned k = y - dilationValue; k < (y + dilationValue ); k++)
					{
						if (circlePredicate(j - x, k - y, dilation2) and compare(newColor, newImage.at(j, k)))
							newImage.setPixel(j, k, newColor);
					}
				}
			}
		}
	}

	std::swap(_data, newImage._data);
	_height = newImage._height;
	_width = newImage._width;
	_maxColor = newImage._maxColor;
	_minColor = newImage._minColor;

	//recalcMinMax();
	if (_minColor != 0)
	{
		for (unsigned i = 0; i < (_width * _height); i++)
		{
			_data[i] -= _minColor;
		}

		_minColor -= _minColor;
		_maxColor -= _minColor;
	}
	// */
	recalcMinMax();
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
void Image::triangle(QVector3D fa, QVector3D fb, QVector3D fc, bool (&compare)(ColorType, ColorType))
{
	// convert to integer vectors with proper rounding
	vec3i a = vec3i::from(fa);
	vec3i b = vec3i::from(fb);
	vec3i c = vec3i::from(fc);

    // sort by y
	if (fa.y() > fb.y())
	{
		std::swap(fa, fb);
        std::swap(a, b);
	}
	if (fa.y() > fc.y())
	{
		std::swap(fa, fc);
        std::swap(a, c);
	}
	if (fb.y() > fc.y())
	{
		std::swap(fb, fc);
        std::swap(b, c);
	}

	// creating the edge vectors
	vec3i ab = b - a;
	vec3i ac = c - a;
	vec3i bc = c - b;
	QVector3D fab = fb - fa;
	QVector3D fac = fc - fa;
	QVector3D fbc = fc - fb;

	//if (fabs(ac.y()) < pixelResolution)
	if (abs(ac.y()) == 0)
        return; // longest edge is too small to make a difference : degenerate triangle, don't draw it

	int t = 0;

	// is AB edge y big enough to draw?
	if (abs(ab.y()) > 0)
	{
        // drawing the spans between AB and AC edges
		for (int y = a.y(); y < b.y(); y++)
        {
			vec3i start =		a      + ab      * t / ab.y();
			vec3i end   =		a      + ac      * t / ac.y();
			double z_start =	fa.z() + fab.z() * t / fab.y();
			double z_end =		fa.z() + fac.z() * t / fac.y();

            if (start.x() > end.x())
			{
				std::swap(z_start, z_end);
                std::swap(start, end);
			}

			vec3i span  = end - start;

			for (int x = start.x(); x < end.x(); x++)
            {
				double progress =  (double)(x - start.x()) / (double)span.x();
				//double newValue1 = (double)start.z() + (double)span.z() * progress;
				double newValue = z_start + (z_end - z_start) * progress;

				/*
				if (fabs (newValue - newValue1) > 2.)
				{
					qDebug() <<  "bla!";
				}
				// */

				if (pixelIsInside(x, y))
                {
					if (not compare(at(x, y), newValue))
						setPixel(x, y, newValue);
                }
            }
			t++;
        }
    }

    // is BC edge too small? then ignore it
	if (abs(bc.y()) > 0)
    {
		double d = 0.;

		// drawing the spans between AB and AC edges
		for (double y = b.y(); y < c.y(); y++)
        {
			vec3i start =		b      + bc * d / bc.y();
			vec3i end   =		a      + ac * t / ac.y();
			double z_start =	fa.z() + fbc.z() * d / fbc.y();
			double z_end =		fa.z() + fac.z() * t / fac.y();

            if (start.x() > end.x())
			{
				std::swap(z_start, z_end);
                std::swap(start, end);
			}

			vec3i span  = end - start;

			for (int x = start.x(); x < end.x(); x ++)
            {
				double progress = (double)(x - start.x()) / (double)span.x();
				//double newValue = (double)start.z() + (double)span.z() * progress;
				double newValue = z_start + (z_end - z_start) * progress;

				if (pixelIsInside(x, y))
                {
					if (not compare(at(x, y), newValue))
						setPixel(x, y, newValue);
                }
            }
			t++;
			d++;
        }
    }
}
