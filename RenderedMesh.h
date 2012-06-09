#pragma once
#include <memory>
#include "image.h"

struct RenderedMesh
{
	/// creates top and bottom z-buffer for the OFF file @param filename.
	/// @param spp determines how fine The z value is determined.
	RenderedMesh(const char* filename, unsigned spp = 10);
	~RenderedMesh();

	void setSamplesPerPixel(unsigned spp);
	void rebuildTop(); /// rebuilds the top image.
	void rebuildBottom(); /// rebuilds the bottom image

	const Mesh*		getMesh() const { return _mesh; }
	unsigned		getSamplesPerPixel() const { return _samplesPerPixel; }
	const Image*	getTop() const { return _top; }
	const Image*	getBottom() const { return _bottom; }

	void scale(QVector3D factor);
	void translate(QVector3D offet);


private:
	Mesh*		_mesh;
	unsigned	_samplesPerPixel;
	Image*		_top;
	Image*		_bottom;
};

typedef std::vector<RenderedMesh*> MeshList;
