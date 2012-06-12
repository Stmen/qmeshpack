#include "Node.h"
#include <memory>
using namespace std;

/////////////////////////////////////////////////////////////////////////////////////////////////////
Node::Node(const char* filename, unsigned spp, unsigned dilation)
	: _samplesPerPixel(spp), _dilation(dilation)
{
	_mesh = new Mesh(filename);
	auto_ptr<Mesh> mesh_guard(_mesh);

	#pragma omp parallel
	{
		_top = new Image(*_mesh, Image::Top, _samplesPerPixel, dilation);
		_bottom = new Image(*_mesh, Image::Bottom, _samplesPerPixel, dilation);
	}

	mesh_guard.release();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
Node::~Node()
{	
	delete _bottom;
	delete _top;
	delete _mesh;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void Node::setSamplesPerPixel(unsigned spp)
{
	if (spp != _samplesPerPixel)
	{
		_samplesPerPixel = spp;
		rebuildTop();
		rebuildBottom();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void Node::rebuildTop()
{
	delete _top;
	_top = 0; // in case destructor is called an top gets deleted
	_top = new Image(*_mesh, Image::Top, _samplesPerPixel);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void Node::rebuildBottom()
{
	delete _bottom;
	_bottom = 0; // in case destructor is called an bottom gets deleted
	_bottom = new Image(*_mesh, Image::Bottom, _samplesPerPixel);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void Node::scaleMesh(QVector3D factor)
{
	_mesh->scale(factor);
	rebuildTop();
	rebuildBottom();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void Node::translateMesh(QVector3D offset)
{
	_mesh->translate(offset);
	rebuildTop();
	rebuildBottom();
}