#pragma once
#include <QImage>
#include "Mesh.h"
#include <vector>

class ImageRegion;
class Image
{    

public:

    enum Mode
    {
		Bad = 0,
		Top = 1,
		Bottom = 2
    };

	typedef float ColorType;

	static bool x_less_than_y(ColorType imageZ, ColorType newZ);
	static bool x_greater_y(ColorType imageZ, ColorType newZ);

	Image(const Mesh &mesh, Mode mode, unsigned dilationValue = 0);
	Image(quint32 width, quint32 height);
	~Image();

	void				clear();
	void				setAllPixelsTo(ColorType value);
	void				setPixel(quint32 x, quint32 y, ColorType pixel);
	inline quint32		getWidth() const { return _width; }
	inline quint32		getHeight() const { return _height; }
	//void		addBorder(unsigned borderSize, ColorType defaultValue);
	inline QString		getName() const { return _name; }
	inline ColorType	at(quint32 x, quint32 y) const { return _data[y * _width + x]; }
	inline ColorType	maxColor() const { return _maxColor; }
	inline ColorType	minColor() const { return _minColor; }
	inline bool			hasPixelAt(quint32 x, quint32 y) const { return _alpha[y * _width + x]; }
	inline bool			pixelIsInside(long x, long y) const { return (x >= 0) and (x < (int)_width) and (y >= 0) and (y < (int)_height); }
	//ColorType	maxColor(unsigned x, unsigned y, unsigned w, unsigned h) const;
	//ColorType	minColor(unsigned x, unsigned y, unsigned w, unsigned h) const;
	QImage			toQImage() const;
	void			insertAt(quint32 x, quint32 y, quint32 z, const Image& other);

	struct offset_info
	{
		quint32 x;
		quint32 y;
		ColorType offset;
		bool	early_rejection;
	};

	offset_info				findMinZDistanceAt(quint32 current_x, quint32 current_y, const Image *bottom, ColorType threshold) const;
	void			recalcMinMax();
	void			drawTriangle(QVector3D fa, QVector3D fb, QVector3D fc, bool (&compare)(ColorType, ColorType));
	void			dilate(int dilationValue, bool (&compare)(ColorType, ColorType));
	ImageRegion		select(quint32 x, quint32 y, quint32 width, quint32 height);
	Image*			operator-(const ImageRegion& imgregion);

private:

	float*				_data;		/// raw pixel data
	unsigned char*		_alpha;		/// an array that denotes if a pixel was set.
	quint32				_width;		/// image width
	quint32				_height;	/// image height
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
	Image::ColorType at(quint32 x, quint32 y) const { return _parent->at(x, y); }

private:


	const Image* _parent;
	unsigned _x, _y, _width, _height;

	ImageRegion(const Image* parent, quint32 x, quint32 y, quint32 width, quint32 height);
};

