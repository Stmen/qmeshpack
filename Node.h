#pragma once
#include <QMatrix4x4>
#include <QVector3D>
#include "Image.h"
#include <functional>

/**
 * creates top and bottom z-buffer for the 3D mesh file.
 *	@param filename: filename of the mesh file.
 *	@param dilation: dilation value for renderings.
 */
class Node
{
public:

	Node(QString filename, unsigned dilation = 10);
	~Node();	

    const Mesh*     getMesh() const { return _mesh; }
    Mesh*           getMesh() { return _mesh; }
	const Image*	getTop() const { return _top; }
	const Image*	getBottom() const { return _bottom; }
	void			scaleMesh(const QVector3D factor);		
	void			setDilationValue(unsigned dil);

	inline void			setPos(QVector3D pos) { _transform.setColumn(3, QVector4D(pos, 1.)); }
	inline QVector3D	getPos() const { return _transform.column(3).toVector3D(); }
	inline unsigned		getDilationValue() const  { return _dilation; }
	inline QMatrix4x4	getTransform() const { return _transform; }
	inline double		getAABBVolume() const { return _mesh->getGeometry().x() * _mesh->getGeometry().y() * _mesh->getGeometry().z(); }
	inline double		getTopBottomVolume() const { return _top->diffSum(*_bottom); }
	struct Orientation
	{
		Image*	top;
		Image*	bottom;
		QMatrix4x4 transform;
	};

private:

	void		rebuildImages();

	Mesh*		_mesh;
	Image*		_top;
	Image*		_bottom;
	unsigned	_dilation;
	QMatrix4x4	_transform;
};
