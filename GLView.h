#pragma once
#include <QGLWidget> // need to include QT += opengl in prject.pro
#include <QVector3D>
#include <QPoint>
#include <QMouseEvent>
#include "Node.h"
#include <QMatrix4x4>
#include <QQuaternion>
#include "MeshFilesModel.h"

class GLView : public QGLWidget
{
	Q_OBJECT // must include this if you use Qt signals/slots

	MeshFilesModel				_singleNodeWrapper;
	const MeshFilesModel&		_nodes;
	QMatrix4x4			_cam;
	QPoint				_mouseLast;
	float				_mouseSensitivity, _moveSensetivity;

public:

	GLView(const MeshFilesModel& nodes, QWidget *parent = NULL);
	 GLView(Node* node, QVector3D geometry, QWidget *parent = NULL);
	 virtual ~GLView();

	 inline QSize minimumSizeHint() const { return QSize(100, 100); }
	 inline QSize sizeHint() const { return QSize(600, 400); }

protected:

	 void initializeGL();
	 void resizeGL(int width, int height);
	 void paintGL();
	 void mousePressEvent(QMouseEvent *event);	 
	 void mouseMoveEvent(QMouseEvent *event);
	 void keyPressEvent(QKeyEvent *event);
	 void wheelEvent(QWheelEvent *event);
};
