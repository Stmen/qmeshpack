#include "Node.h"
#include <memory>
#include <QtConcurrentRun>
using namespace std;

/////////////////////////////////////////////////////////////////////////////////////////////////////
Node::Node(QString filename, unsigned dilation)	:
	_top(0),
	_bottom(0),
	_dilation(dilation)
{
    _mesh = new Mesh(filename.toUtf8().constData());
	auto_ptr<Mesh> mesh_guard(_mesh);
	rebuildImages();
	mesh_guard.release();
	_transform.setToIdentity();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
Node::~Node()
{	
	delete _mesh;
	delete _top;
	delete _bottom;		
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void Node::rebuildImages()
{
	QFuture<Image*> futureTop =  QtConcurrent::run([this](){return new Image(*_mesh, Image::Top, _dilation);});
	QFuture<Image*> futureBottom =  QtConcurrent::run([this](){return new Image(*_mesh, Image::Bottom, _dilation);});
	if (_top)
	{
		delete _top;
		delete _bottom;
	}
	_top = futureTop.result();
	_bottom = futureBottom.result();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void Node::scaleMesh(QVector3D factor)
{
	_mesh->scale(factor);
	rebuildImages();
}
