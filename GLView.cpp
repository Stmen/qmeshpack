#include <QDebug>
#include <QFileDialog>
#include "GLView.h"
#include "config.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////
GLView::GLView(bool use_lighting, QWidget* parent) :
	QGLWidget(parent),
	_nodes(0),
	_mouseLast(0, 0),
	_rotationSensitivity(0.001),
	_moveSensetivity(0.4),
	_wheelSensitivity(0.6),
	_drawPackBox(false),
    _single(false),
    _useLighting(use_lighting)
{
	setFocusPolicy(Qt::StrongFocus);
	setAutoBufferSwap(false);
	_mouseLast = QPoint(0., 0.);
	_cam.setToIdentity();
	//connect(&_singleNodeWrapper, SIGNAL(geometryChanged()), this, SLOT(updateGL()));    
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
GLView::GLView(NodeModel *nodes, bool use_lighting, QWidget *parent) :
	QGLWidget(parent),	
	_nodes(nodes),
	_mouseLast(0, 0),
	_rotationSensitivity(0.001),
	_moveSensetivity(0.2),
	_wheelSensitivity(0.6),
	_drawPackBox(true),
    _single(false),
    _useLighting(use_lighting)
{
	setFocusPolicy(Qt::StrongFocus);
	setAutoBufferSwap(false);
	_mouseLast = QPoint(0., 0.);
	_cam.setToIdentity();
	QVector3D pos(nodes->getGeometry().x() / 2, nodes->getGeometry().y() / 2, nodes->getGeometry().z() * 2);
	_cam.translate(pos);
	_lightPos = QVector4D(pos, 1.);
	connect(nodes, SIGNAL(geometryChanged()), this, SLOT(updateGL()));
	//connect(&_singleNodeWrapper, SIGNAL(geometryChanged()), this, SLOT(updateGL()));
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
GLView::~GLView()
{
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void GLView::setNode(Node *node)
{
	_drawPackBox = false;
	_single = true;
	_node = node;
	_cam.setToIdentity();
    QVector3D geom = node->getMesh()->getGeometry();
	QVector3D pos = node->getPos() + QVector3D(geom.x() / 2, geom.y() / 2, geom.z() * 2);
    _cam.translate(pos);
	_lightPos = QVector4D(pos, 1.);

	if (isValid())
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

	//glFrontFace(GL_CCW);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_CULL_FACE);

	// Create light components
	GLfloat ambientLight[] = { 0.2f, 0.2f, 0.2f, 1.0f };
	GLfloat diffuseLight[] = { 0.9, 0.9, 0.9, 1.f };
	GLfloat specularLight[] = { 0.5f, 0.5f, 0.5f, 1.0f };	

	// Assign created components to GL_LIGHT0
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
	glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);
	glLightfv(GL_LIGHT0, GL_POSITION, (float*)&_lightPos);

	//GLfloat position[] = { -1.5f, 1.0f, -4.0f, 1.0f };
	//glLightfv(GL_LIGHT0, GL_POSITION, (GLfloat*)&_lightPos);

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
	glFrustum(-(float)width * 0.001, (float)width *  0.001, -(float)height *  0.001, (float)height *  0.001, 1., 10000);
	glMatrixMode(GL_MODELVIEW);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void GLView::flushGL() const
{
	if (isValid())
		glFlush();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void GLView::saveScreenshot()
{
	if (not isValid())
		return;

	flushGL();
	QImage framebuffer = grabFrameBuffer(false);

	QString filename = QFileDialog::getSaveFileName(this,
		tr("Save screenshot"), "",
		tr("PNG (*.png);; JPG (*.jpg);; All Files (*)"));

	if (filename.isEmpty())
		return;

	// what if the users did not specify a suffix...?
	QFileInfo fileInfo(filename);
	if (fileInfo.suffix().isEmpty())
		filename += ".png";

	framebuffer.save(filename);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void GLView::paintGL()
{
	if (not isValid())
		return;

	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixd(_cam.inverted().constData());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnableClientState(GL_VERTEX_ARRAY);
    if (_useLighting)
    {
        glEnableClientState(GL_NORMAL_ARRAY);
        glEnable(GL_LIGHTING);
        float ligtPos[4] = { (float)_cam.column(3).x(), (float)_cam.column(3).y(), (float)_cam.column(3).z(), 1 };
        glLightfv(GL_LIGHT0, GL_POSITION, ligtPos);
    }
    else
    {
        glDisableClientState(GL_NORMAL_ARRAY);
        glDisable(GL_LIGHTING);
        glColor3f(1., 0., 0.);
    }

	if (_single)
	{
		QVector3D nodePos = _node->getPos();
		glTranslatef(nodePos.x(), nodePos.y(), nodePos.z());
        _node->getMesh()->draw(_useLighting);
	}
	else
	{
		QVector3D min(INFINITY, INFINITY, INFINITY), max(-INFINITY, -INFINITY, -INFINITY);
		for (unsigned i = 0; i < _nodes->numNodes(); i++)
		{
			Node* node = _nodes->getNode(i);
			QVector3D nodePos = node->getPos();

			glPushMatrix();
            min = vecmin(min, nodePos + node->getMesh()->getMin());
            max = vecmax(max, nodePos + node->getMesh()->getMax());

			glTranslatef(nodePos.x(), nodePos.y(), nodePos.z());

            _nodes->getNode(i)->getMesh()->draw(_useLighting);
			glPopMatrix();
		}

        if (_useLighting)
            glDisable(GL_LIGHTING);

		if (_drawPackBox)
		{
			glColor3f(1., 1., 0.);			
			drawAxisAlignedBox(QVector3D(0., 0., 0.), _nodes->getGeometry());			
		}
	}

	glDisableClientState(GL_VERTEX_ARRAY);

    if (_useLighting)
    {
        glDisableClientState(GL_NORMAL_ARRAY);
    }

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
	_mouseLast = event->pos();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void GLView::mouseMoveEvent(QMouseEvent *event)
{
	if (not isValid())
		return;

	QPoint delta = event->pos() - _mouseLast;
	_mouseLast = event->pos();
	QMatrix4x4 mat;
	mat.setToIdentity();

	if(event->buttons() & Qt::LeftButton)
	{
        QVector3D origin, geometry;
        if(_single)
        {
            origin = _node->getPos();
            geometry = _node->getMesh()->getGeometry();
        }
        else
        {
            origin = QVector3D(0., 0., 0.);
            geometry = _nodes->getGeometry();
        }

		//geometry = QVector3D(geometry.x(), geometry.y(), geometry.z());
		QVector3D offset = origin + geometry / 2;

#ifdef CAMERA_CENTERED
		mat.translate(offset);
		if (delta.x())
			mat.rotate(delta.x(), _cam.column(1).toVector3D());
		if (delta.y())
			mat.rotate(delta.y(), _cam.column(0).toVector3D());
		mat.translate(-offset);
		_cam = mat * _cam; // camera centered on (0. 0. 0.)
#else
		if (delta.x())
			mat.rotate(-delta.x(), _cam.column(1).toVector3D());
		if (delta.y())
			mat.rotate(-delta.y(), _cam.column(0).toVector3D());
		//_cam.translate(offset);
		_cam = _cam * mat;
#endif
	}
	else if (event->buttons() & Qt::RightButton)
	{
		if (delta.y() != 0)
			mat.translate(_cam.column(1).toVector3D() * _moveSensetivity * -delta.y());
		if (delta.x() != 0)
			mat.translate(_cam.column(0).toVector3D() * _moveSensetivity * delta.x());
		_cam = mat * _cam;

	}

	updateGL();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void GLView::keyPressEvent(QKeyEvent* event)
{
	if (not isValid())
		return;

	QMatrix4x4 mat;
	mat.setToIdentity();
	bool camOp = false;

	switch(event->key())
	{	
		case Qt::Key_W:
			camOp = true;
			mat.translate(_cam.column(1).toVector3D() * _moveSensetivity * 4.);
			updateGL();
			break;

		case Qt::Key_S:
			camOp = true;
			mat.translate(_cam.column(1).toVector3D() * -_moveSensetivity* 4.);
			updateGL();
			break;

		case Qt::Key_A:
			camOp = true;
			mat.translate(_cam.column(0).toVector3D() * -_moveSensetivity* 4.);
			updateGL();
			break;

		case Qt::Key_D:
			camOp = true;
			mat.translate(_cam.column(0).toVector3D() * _moveSensetivity* 4.);
			updateGL();
			break;

		case Qt::Key_Space:
			camOp = true;
			mat.translate(_cam.column(2).toVector3D() * -_moveSensetivity * 4.);
			updateGL();
			break;

		case Qt::Key_Shift:
			camOp = true;
			mat.translate(_cam.column(2).toVector3D() * -_moveSensetivity * 4.);
			updateGL();
			break;

		case Qt::Key_F2:
			saveScreenshot();
			break;

		default:
			event->ignore();
			return;
	}	

	if (camOp)
		_cam = mat * _cam;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void GLView::wheelEvent(QWheelEvent *event)
{
	if (not isValid())
		return;

	_cam.translate(0., 0., (float)event->delta() * _wheelSensitivity);
	updateGL();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void GLView::setUseLighting(bool enable_lighting)
{
    _useLighting = enable_lighting;
}
