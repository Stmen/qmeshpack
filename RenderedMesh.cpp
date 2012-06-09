#include "RenderedMesh.h"
#include <memory>
using namespace std;

/////////////////////////////////////////////////////////////////////////////////////////////////////
RenderedMesh::RenderedMesh(const char* filename, unsigned spp)
	: _samplesPerPixel(spp)
{
	_mesh = new Mesh(filename);
	auto_ptr<Mesh> mesh_guard(_mesh);

	_top = new Image(*_mesh, Image::Top, _samplesPerPixel);
	auto_ptr<Image> image_guard(_top);

	_bottom = new Image(*_mesh, Image::Bottom, _samplesPerPixel);

	image_guard.release();
	mesh_guard.release();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
RenderedMesh::~RenderedMesh()
{
	delete _mesh;
	delete _top;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void RenderedMesh::setSamplesPerPixel(unsigned spp)
{
	if (spp != _samplesPerPixel)
	{
		_samplesPerPixel = spp;
		rebuildTop();
		rebuildBottom();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void RenderedMesh::rebuildTop()
{
	delete _top;
	_top = 0; // in case destructor is called an top gets deleted
	_top = new Image(*_mesh, Image::Top, _samplesPerPixel);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void RenderedMesh::rebuildBottom()
{
	delete _bottom;
	_bottom = 0; // in case destructor is called an bottom gets deleted
	_bottom = new Image(*_mesh, Image::Bottom, _samplesPerPixel);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void RenderedMesh::scale(QVector3D factor)
{
	_mesh->scale(factor);
	rebuildTop();
	rebuildBottom();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void RenderedMesh::translate(QVector3D offset)
{
	_mesh->translate(offset);
	rebuildTop();
	rebuildBottom();
}
