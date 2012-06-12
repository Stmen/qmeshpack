#pragma once
#include <QMatrix4x4>
#include <QVector3D>
#include "image.h"

struct Node
{
	/// creates top and bottom z-buffer for the OFF file @param filename.
	/// @param spp determines how fine The z value is determined.
	Node(const char* filename, unsigned spp = 10, unsigned dilation = 10);
	~Node();

	void setSamplesPerPixel(unsigned spp);
	void rebuildTop(); /// rebuilds the top image.
	void rebuildBottom(); /// rebuilds the bottom image

	const Mesh*		getMesh() const { return _mesh; }
	unsigned		getSamplesPerPixel() const { return _samplesPerPixel; }
	const Image*	getTop() const { return _top; }
	const Image*	getBottom() const { return _bottom; }
	void			scaleMesh(const QVector3D factor);
	void			translateMesh(const QVector3D offset);
	void			setPos(QVector3D pos) { _transform.setColumn(3, QVector4D(pos, 1.)); }
	QVector3D		getPos() const { return _transform.column(3).toVector3D(); }
	unsigned		getDilationValue() const  { return _dilation; }

private:

	Mesh*		_mesh;
	unsigned	_samplesPerPixel;
	Image*		_top;
	Image*		_bottom;
	unsigned	_dilation;
	QMatrix4x4	_transform;
};
