#include "Image.h"
#include <QDebug>
#include <cassert>
#include <cstring>
#include <algorithm>
#include <iostream>
#include "util.h"
#include "Exception.h"
using namespace std;

/////////////////////////////////////////////////////////////////////////////////////////////////////
Image::Image(quint32 width, quint32 height) :
	_width(width),
	_height(height),
	_minColor(INFINITY),
	_maxColor(-INFINITY)
{	
	if (width == 0 or height == 0)
		THROW(ImageException, "bad geometry");

    _data = new ColorType[width * height];
	_alpha = new unsigned char[width * height];
	memset(_alpha, 0, width * height);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
Image::Image(const Mesh& mesh, Mode mode, unsigned dilationValue) :
	_minColor(INFINITY),
	_maxColor(-INFINITY)
{
	QVector3D geometry = mesh.getGeometry();
	_width = geometry.x();
	_height = geometry.y();
	_name = mesh.getName();

	if (geometry.x() < 1. or geometry.y() < 1. or geometry.z() < 1.)
	{
		THROW(ImageException, QString("mesh \"%1\" is too small, its dimensions are: (%2, %3, %4), scale it up.").arg(mesh.getName(),
						QString::number(geometry.x()), QString::number(geometry.y()), QString::number(geometry.z())));
	}

	size_t numPixels = _width * _height;
	_data = new ColorType[numPixels];
	_alpha = new unsigned char[numPixels];
	memset(_alpha, 0, numPixels);

    switch (mode)
    {
		case Top:
			_name += "_top";
			for (Mesh::Iterator it = mesh.vertexIterator(); it.is_good(); it.next())
			{
				Triangle tri = it.get();
				drawTriangle(tri.vertex[0] - mesh.getMin(),
							 tri.vertex[1] - mesh.getMin(),
							 tri.vertex[2] - mesh.getMin(),
							 Image::x_greater_y);
			}						
			dilate(dilationValue, Image::x_greater_y);
			break;

		case Bottom:
			_name += "_bottom";
			for (Mesh::Iterator it = mesh.vertexIterator(); it.is_good(); it.next())
			{
				Triangle tri = it.get();
				drawTriangle(tri.vertex[0] - mesh.getMin(), tri.vertex[1] - mesh.getMin(), tri.vertex[2] - mesh.getMin(),
						 Image::x_less_than_y);
			}
			dilate(dilationValue, Image::x_less_than_y);
			break;

		default:
			THROW(ImageException, "bad drawing mode");
    }		
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
Image::~Image()
{
	delete [] _data;
	delete [] _alpha;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void Image::clear()
{	
	memset(_alpha, 0, _height * _width);
	_minColor = INFINITY;
	_maxColor = -INFINITY;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void Image::setAllPixelsTo(ColorType value)
{
	_maxColor = _minColor = value;
	size_t numPixels = _height * _width;
	for (unsigned i = 0; i < numPixels; i++)
		_data[i] = value;

	memset(_alpha, 1, numPixels);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void Image::setPixel(quint32 x, quint32 y, ColorType color)
{
	assert(x < _width and y < _height);
	quint32 idx = y * _width + x;
	_data[idx] = color;
	_alpha[idx] = 1;
	_minColor = std::min(_minColor, color);
	_maxColor = std::max(_maxColor, color);
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
			unsigned idx = y * _width + x;
			unsigned rgbColor;
			if (_alpha[idx])
			{
				int color8 = (_data[idx] - _minColor) * colorStep;
				assert(color8 >= 0 and color8 < 256);
				rgbColor =  (color8 << 16) | (color8 << 8) | color8;
			}
			else
				rgbColor = (59U << 16) | (110U << 8) | (165U);

            image.setPixel(x, y, rgbColor);
        }
    }

    return image;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
bool Image::x_less_than_y(ColorType imageZ, ColorType newZ)
{
    return imageZ < newZ;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
bool Image::x_greater_y(ColorType imageZ, ColorType newZ)
{
    return imageZ > newZ;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void Image::insertAt(quint32 x, quint32 y, quint32 z, const Image &other)
{
	if ((x + other.getWidth() > _width) or (y + other.getHeight() > _height))
		THROW(ImageException, "images overlap");

	for (quint32 other_y = 0; other_y < other.getHeight(); other_y++)
	{
		for (quint32 other_x = 0; other_x < other.getWidth(); other_x++)
		{
			if (other.hasPixelAt(other_x, other_y))
			{
				ColorType other_z = other.at(other_x, other_y);
				setPixel(x + other_x, y + other_y, z + other_z);
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
Image::offset_info Image::findMinZDistanceAt(quint32 current_x, quint32 current_y, const Image* bottom, ColorType threshold) const
{
	if ((current_x + bottom->getWidth() > _width) or (current_y + bottom->getHeight() > _height))
		THROW(ImageException, "images overlap");

	Image::ColorType min_z = INFINITY;
	quint32 min_x = 0;
	quint32 min_y = 0;
	bool early_rejection = false;

	// TODO : use SSE
	for (quint32 y = 0; y < bottom->getHeight(); y++)
	{
		for (quint32 x = 0; x < bottom->getWidth(); x++)
		{
			//if (hasPixelAt(current_x + x, current_y + y) and // base image has pixels everywhere
			if(bottom->hasPixelAt(x, y))
			{
				Image::ColorType dist = bottom->at(x, y) - at(current_x + x, current_y + y);
				if (dist < min_z)
				{
					min_z = dist;
					min_x = x;
					min_y = y;
				}
			}
		}
	}

	assert(min_z != INFINITY && "impossible since at least base image has minimum height everywhere." );

	offset_info info = {min_x, min_y, min_z, early_rejection};
	return info;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void Image::recalcMinMax()
{
	_minColor = INFINITY;
	_maxColor = -INFINITY;

	for (quint32 i = 0; i < (_width * _height); i++)
	{
		if (_alpha[i])
		{
			_minColor = std::min(_minColor, _data[i]);
			_maxColor = std::max(_maxColor, _data[i]);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
static inline bool circlePredicate(int x, int y, unsigned radius2)
{
	return (unsigned)(x * x + y * y) < radius2;
	//return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void Image::dilate(int dilationValue, bool (&compare)(ColorType, ColorType))
{
	if (dilationValue == 0)
		return;

	//Image newImage(dilationValue * 2 + _width, dilationValue * 2 + _height, clearColor);
	Image newImage(dilationValue * 2 + _width, dilationValue * 2 + _height);
	unsigned dilation2 = dilationValue * dilationValue;

	for (quint32 x = 0; x < _width; x++)
	{
		for (quint32 y = 0; y < _height; y++)
		{			
			if (hasPixelAt(x, y))
			{
				ColorType color = at(x, y);
				ColorType newColor;
				if (compare == x_greater_y)
					newColor = color + dilationValue;
				else if (compare == x_less_than_y)
					newColor = color - dilationValue;
				else
					THROW(ImageException, "unknown compare func.");

				for (int dil_y = -(int)dilationValue; dil_y < (int)dilationValue; dil_y++)
				{
					for (int dil_x = -(int)dilationValue; dil_x < (int)dilationValue; dil_x++)
					{
						int new_x = (int)x + (int)dilationValue + dil_x;
						int new_y = (int)y + (int)dilationValue + dil_y;
						if (	circlePredicate(dil_x, dil_y, dilation2) and // inside a circle
								(not newImage.hasPixelAt(new_x, new_y) or // no pixel at this place
								 compare(newColor, newImage.at(new_x, new_y)))) // new pixel is better
						{
							newImage.setPixel(new_x, new_y, newColor);
						}
					}
				}
			}
		}
	}

	std::swap(_data, newImage._data);
	std::swap(_alpha, newImage._alpha);
	_width = newImage._width;
	_height = newImage._height;
	_minColor = newImage._minColor;
	_maxColor = newImage._maxColor;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
void Image::drawTriangle(QVector3D fa, QVector3D fb, QVector3D fc, bool (&compare)(ColorType, ColorType))
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

				assert (pixelIsInside(x, y));
                {
					if (not hasPixelAt(x, y) or not compare(at(x, y), newValue))
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
		for (int y = b.y(); y < c.y(); y++)
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

				assert (pixelIsInside(x, y));
                {
					if (not hasPixelAt(x, y) or not compare(at(x, y), newValue))
						setPixel(x, y, newValue);
                }
            }
			t++;
			d++;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
Image* Image::operator -(const ImageRegion& other)
{
	assert(_width == other.getWidth());
	assert(_height == other.getHeight());

	Image* img = new Image(_width, _height);
	size_t numPixels = _width * _height;
	memcpy(img->_data, _data, numPixels * sizeof(_data[0]));
	memcpy(img->_alpha, _alpha, numPixels * sizeof(_alpha[0]));

	for (quint32 y = 0; y < _height; y++)
	{
		for (quint32 x = 0; x < _width; x++)
		{
			Image::ColorType color = other.at(x, y);
			img->_minColor = std::min(img->_minColor, color);
			img->_maxColor = std::min(img->_maxColor, color);
			img->_data[y * _width + x] -= color;
		}
	}

	return img;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
ImageRegion	Image::select(quint32 x, quint32 y, quint32 width, quint32 height)
{
	assert(x < width and y < height and width <= _width and height <= _height);
	return ImageRegion(this, x, y, width, height);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
ImageRegion::ImageRegion(const Image* parent, quint32 x, quint32 y, quint32 width, quint32 height) :
	_parent(parent), _x(x), _y(y), _width(width), _height(height)
{
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
Image* ImageRegion::operator +(const Image* other)
{
	Image* img = new Image(_width, _height);
	for (unsigned y = 0; y < _height; y++)
	{
		for (unsigned x = 0; x < _width; x++)
		{
			Image::ColorType color = other->at(x, y);
			img->_minColor = std::min(img->_minColor, color);
			img->_maxColor = std::min(img->_maxColor, color);
			img->setPixel(x, y, _parent->at(_x + x, _y + y) + color);
		}
	}
	return img;
}
