#pragma once
#include <QImage>
#include "mesh.h"
#include <vector>

class ImageRegion;
class Image
{    

public:

    enum Mode
    {
        Top = 0,
        Bottom
    };

	typedef float ColorType;

	static bool x_less_than_y(ColorType imageZ, ColorType newZ);
	static bool x_greater_y(ColorType imageZ, ColorType newZ);

	Image(const Mesh &mesh, Mode mode, unsigned dilationValue = 0);
	Image(unsigned width, unsigned height);
    ~Image();
	void		clear();
	void		setAllPixelsTo(ColorType value);
	bool		pixelIsInside(int x, int y);
	void		setPixel(unsigned x, unsigned y, ColorType pixel);
	unsigned	getWidth() const { return _width; }
	unsigned	getHeight() const { return _height; }
	QImage		toQImage() const;

	//void		addBorder(unsigned borderSize, ColorType defaultValue);
	inline QString		getName() const { return _name; }
	inline ColorType	at(unsigned x, unsigned y) const { return _data[y * _width + x]; }
	inline ColorType	maxColor() const { return _maxColor; }
	inline ColorType	minColor() const { return _minColor; }
	inline bool			hasPixelAt(unsigned x, unsigned y) const { return _alpha[y * _width + x]; }

	//ColorType	maxColor(unsigned x, unsigned y, unsigned w, unsigned h) const;
	//ColorType	minColor(unsigned x, unsigned y, unsigned w, unsigned h) const;
	void		insertAt(unsigned x, unsigned y, unsigned z, const Image& other);
	float		computeMinZDistance(unsigned current_x, unsigned current_y, const Image &bottom) const;
	void		recalcMinMax();
	void		drawTriangle(QVector3D fa, QVector3D fb, QVector3D fc, bool (&compare)(ColorType, ColorType));


	void		dilate(int dilationValue, bool (&compare)(ColorType, ColorType));
	ImageRegion	select(unsigned x, unsigned y, unsigned width, unsigned height);
	Image*		operator-(const ImageRegion& imgregion);

private:

	float*				_data;		/// raw pixel data
	std::vector<bool>	_alpha;		/// an array that denotes if a pixel was set.
	unsigned			_width;		/// image width
	unsigned			_height;	/// image height
	float				_minColor;	/// miminum color of this image
	float				_maxColor;	/// maximum color of this image
	QString				_name;		/// image name

	friend class ImageRegion;
};

class ImageRegion
{
public:
	friend class Image;

	Image* operator+(const Image* other);

	unsigned x() const { return _x; }
	unsigned y() const { return _y; }
	unsigned getWidth() const { return _width; }
	unsigned getHeight() const { return _height; }
	Image::ColorType at(unsigned x, unsigned y) const { return _parent->at(x, y); }

private:


	const Image* _parent;
	unsigned _x, _y, _width, _height;

	ImageRegion(const Image* parent, unsigned x, unsigned y, unsigned width, unsigned height);
};

