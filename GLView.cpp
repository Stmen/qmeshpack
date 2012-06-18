#include "GLView.h"
#include "config.h"
#include <QDebug>

/////////////////////////////////////////////////////////////////////////////////////////////////////
GLView::GLView(QWidget* parent) :
	QGLWidget(parent),
	_nodes(&_singleNodeWrapper),
	_mouseLast(0, 0),
	_mouseSensitivity(0.01),
	_moveSensetivity(10.),
	_wheelSensitivity(0.6)
{
	setFocusPolicy(Qt::StrongFocus);
	setAutoBufferSwap(false);
	_cam.setToIdentity();
	connect(&_singleNodeWrapper, SIGNAL(geometryChanged()), this, SLOT(updateGL()));
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
GLView::GLView(MeshFilesModel* nodes, QWidget* parent) :
	QGLWidget(parent),	
	_singleNodeWrapper(nodes->getGeometry()),
	_nodes(nodes),
	_mouseLast(0, 0),
	_mouseSensitivity(0.01),
	_moveSensetivity(10.),
	_wheelSensitivity(0.6)
{
	setFocusPolicy(Qt::StrongFocus);
	setAutoBufferSwap(false);
	_cam.setToIdentity();
	_cam.translate(QVector3D(nodes->getGeometry().x() / 2, nodes->getGeometry().y() / 2, nodes->getGeometry().z() * 2.4));
	connect(&_singleNodeWrapper, SIGNAL(geometryChanged()), this, SLOT(updateGL()));
	connect(nodes, SIGNAL(geometryChanged()), this, SLOT(updateGL()));
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
GLView::GLView(Node* node, QVector3D geometry, QWidget *parent) :
	QGLWidget(parent),
	_singleNodeWrapper(geometry),
	_nodes(&_singleNodeWrapper),
	_mouseLast(0, 0),
	_mouseSensitivity(0.01),
	_moveSensetivity(0.1)
{
	_singleNodeWrapper.addNode(node);
	setFocusPolicy(Qt::StrongFocus);
	setAutoBufferSwap(false);
	_cam.setToIdentity();
	_cam.translate(QVector3D(geometry.x() / 2, geometry.y() / 2, geometry.z() * 2.4));
	connect(&_singleNodeWrapper, SIGNAL(geometryChanged()), this, SLOT(updateGL()));
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
GLView::~GLView()
{
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void GLView::setNode(Node* node, QVector3D geometry)
{
	_singleNodeWrapper.clear();
	_singleNodeWrapper.setGeometry(geometry);
	_singleNodeWrapper.addNode(node);
	_nodes = &_singleNodeWrapper;
	_cam.setToIdentity();
	_cam.translate(QVector3D(geometry.x() / 2, geometry.y() / 2, geometry.z() * 4));
	updateGL();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void GLView::initializeGL()
{
	if (not isValid())
		return;

	glDisable(GL_TEXTURE_2D);
	//glDisable(GL_COLOR_MATERIAL);
	glEnable(GL_DEPTH_TEST);	
	glEnable(GL_BLEND);
	glEnable(GL_POLYGON_SMOOTH);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glShadeModel(GL_SMOOTH);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(59./256., 110. / 256., 165./256., 0.);


#ifdef USE_LIGHTING
	//glFrontFace(GL_CCW);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	// Create light components
	GLfloat ambientLight[] = { 0.2f, 0.2f, 0.2f, 1.0f };
	GLfloat diffuseLight[] = { 0.8, 0.8, 0.8, 1.f };
	GLfloat specularLight[] = { 0.5f, 0.5f, 0.5f, 1.0f };	
	_lightPos = QVector4D(0., 0., _nodes->getGeometry().z(), 1);
	// Assign created components to GL_LIGHT0
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
	glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);
	glLightfv(GL_LIGHT0, GL_POSITION, (float*)&_lightPos);

	//GLfloat position[] = { -1.5f, 1.0f, -4.0f, 1.0f };
	//glLightfv(GL_LIGHT0, GL_POSITION, (GLfloat*)&_lightPos);
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void GLView::resizeGL(int width, int height)
{
	if (not isValid())
		return;

	//int side = qMin(width, height);
	//glViewport((width - side) / 2, (height - side) / 2, side, side);
	glViewport(0, 0, width, height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-(float)width * 0.001, (float)width *  0.001, -(float)height *  0.001, (float)height *  0.001, 1., 5000);
	glMatrixMode(GL_MODELVIEW);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void GLView::paintGL()
{
	if (not isValid())
		return;

	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixd(_cam.inverted().constData());

#ifdef USE_LIGHTING
    glEnable(GL_LIGHTING);
	glLightfv(GL_LIGHT0, GL_POSITION, (float*)&_lightPos);    
#endif

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	QVector3D min(INFINITY, INFINITY, INFINITY), max(-INFINITY, -INFINITY, -INFINITY);
	glColor3f(1., 0., 0.);

	for (unsigned i = 0; i < _nodes->numNodes(); i++)
	{
		Node* node = _nodes->getNode(i);
		QVector3D nodePos = node->getPos();

		glPushMatrix();
		min = vecmin(min, nodePos + node->getMesh()->getMin());
		max = vecmax(max, nodePos + node->getMesh()->getMax());

		glTranslatef(nodePos.x(), nodePos.y(), nodePos.z());

		_nodes->getNode(i)->getMesh()->draw(false);
		glPopMatrix();
	}

#ifdef USE_LIGHTING
    glDisable(GL_LIGHTING);
#endif
	glColor3f(1., 1., 0.);
	glEnableClientState(GL_VERTEX_ARRAY);
	drawAxisAlignedBox(QVector3D(0., 0., 0.), _nodes->getGeometry());
	glDisableClientState(GL_VERTEX_ARRAY);

	//*/

/* test triangle
	glBegin(GL_TRIANGLES);
	glVertex3f(-5,-5, 15);
	glVertex3f(5, -5, 15);
	glVertex3f(0, 5, 15);
	glEnd();
	//*/
	swapBuffers();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void GLView::mousePressEvent(QMouseEvent* event)
{
	//qDebug() << "pressed " << event->button();
	if(event->button() & Qt::LeftButton)
	{		
		_mouseLast = event->pos();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void GLView::mouseMoveEvent(QMouseEvent *event)
{
	if(event->buttons() & Qt::LeftButton)
	{
		QPoint diff = event->pos() - _mouseLast;
		//qDebug() << "mouseMove " << diff;
		QMatrix4x4 mat;
		mat.setToIdentity();				
		mat.translate(_nodes->getGeometry() / 2.);
		mat.rotate(diff.x(), _cam.column(1).toVector3D());
		mat.rotate(diff.y(), _cam.column(0).toVector3D());
		mat.translate(_nodes->getGeometry() / -2.);

		//_cam.translate(_nodes->getGeometry() / -2.);
		_cam = mat * _cam; // camera centered on (0. 0. 0.)
		//_cam.translate(_nodes->getGeometry() / 2.);

		//_cam.rotate(diff.x(), 0., 1., 0.);
		//_cam.rotate(diff.y(), 1., 0., 0.);
		_mouseLast = event->pos();
		updateGL();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void GLView::keyPressEvent(QKeyEvent* event)
{
	switch(event->key())
	{	
		case Qt::Key_W:
			_cam.translate(_cam.column(1).toVector3D() * _moveSensetivity);
			updateGL();
			break;

		case Qt::Key_S:
			_cam.translate(_cam.column(1).toVector3D() * -_moveSensetivity);
			updateGL();
			break;

		case Qt::Key_A:
			_cam.translate(_cam.column(0).toVector3D() * -_moveSensetivity);
			updateGL();
			break;

		case Qt::Key_D:
			_cam.translate(_cam.column(0).toVector3D() * _moveSensetivity);
			updateGL();
			break;

		case Qt::Key_Space:			
			_cam.translate(_cam.column(2).toVector3D() * -_moveSensetivity);
			updateGL();
			break;

		case Qt::Key_Shift:
			_cam.translate(_cam.column(2).toVector3D() * -_moveSensetivity);
			updateGL();
			break;

		default:
			event->ignore();
			return;
	}	
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void GLView::wheelEvent(QWheelEvent *event)
{
	_cam.translate(0., 0., (float)event->delta() * _wheelSensitivity);
	updateGL();
}
