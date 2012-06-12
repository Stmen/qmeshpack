#pragma once
#include <QImage>
#include "mesh.h"

class Image
{    

public:

    enum Mode
    {
        Top = 0,
        Bottom
    };

	typedef float ColorType;

	static bool minValue(ColorType imageZ, ColorType newZ);
	static bool maxValue(ColorType imageZ, ColorType newZ);

	Image(const Mesh &mesh, Mode mode, unsigned samples_per_pixel = 10, unsigned dilationValue = 0);
	Image(unsigned width, unsigned height, ColorType clearColor = 0.);
    ~Image();

	unsigned	getWidth() const { return _width; }
	unsigned	getHeight() const { return _height; }
	QImage		toQImage() const;
	void		clear(ColorType value = 0.);
	ColorType	at(unsigned x, unsigned y) const { return _data[y * _width + x]; }
	void		setPixel(unsigned x, unsigned y, ColorType pixel);
	bool		pixelIsInside(float x, float y);
	void		addBorder(unsigned borderSize, ColorType defaultValue);
	ColorType	maxColor() const { return _maxColor; }
	ColorType	minColor() const { return _minColor; }
	ColorType	maxColor(unsigned x, unsigned y, unsigned w, unsigned h) const;
	ColorType	minColor(unsigned x, unsigned y, unsigned w, unsigned h) const;
	void		insertAt(unsigned x, unsigned y, unsigned z, const Image& other);
	float		computeMinZ(unsigned x, unsigned y, const Image &other);
	void		triangle(QVector3D a, QVector3D b, QVector3D c, bool (&compare)(ColorType, ColorType), unsigned samples_per_pixel = 10);
	QString		getName() const { return _name; }
	void		recalcMinMax();
	void		dilate(int dilationValue, bool (&compare)(ColorType, ColorType));

private:
	unsigned    _width;
	unsigned    _height;
	float*      _data;
	float       _minColor;
	float       _maxColor;
	QString		_name;
};

