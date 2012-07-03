#pragma once
#include <QMatrix4x4>
#include <QVector3D>
#include "Image.h"

struct Node
{
	/// creates top and bottom z-buffer for the OFF file @param filename.
	/// @param spp determines how fine The z value is determined.
	Node(QString filename, unsigned dilation = 10);
	~Node();

	void rebuildImages(); /// rebuilds the top image.

    const Mesh*     getMesh() const { return _mesh; }
    Mesh*           getMesh() { return _mesh; }
	const Image*	getTop() const { return _top; }
	const Image*	getBottom() const { return _bottom; }
	void			scaleMesh(const QVector3D factor);
	void			translateMesh(const QVector3D offset);
    void			setPos(QVector3D pos) { _pos = pos; }
    QVector3D		getPos() const { return _pos; }
	unsigned		getDilationValue() const  { return _dilation; }

private:

	Mesh*		_mesh;
	Image*		_top;
	Image*		_bottom;
	unsigned	_dilation;
    QVector3D   _pos;
};
