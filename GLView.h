#pragma once
#include <QGLWidget> // need to include QT += opengl in prject.pro
#include <QVector3D>
#include <QPoint>
#include <QMouseEvent>
#include "Node.h"
#include <QMatrix4x4>
#include "NodeModel.h"

class GLView : public QGLWidget
{
	Q_OBJECT // must include this if you use Qt signals/slots

	union
	{
		const NodeModel*	_nodes;
		const Node*			_node;
	};

	QMatrix4x4			_cam;
	QPoint				_mouseLast;
	float				_rotationSensitivity, _moveSensetivity, _wheelSensitivity;
	QVector4D			_lightPos;
	bool				_drawPackBox;
	bool				_single;
    bool                _useLighting;

public:

    GLView(bool use_lighting, QWidget* parent = NULL);
	GLView(const NodeModel *nodes, bool use_lighting, QWidget* parent = NULL);
	virtual ~GLView();

	inline QSize minimumSizeHint() const { return QSize(100, 100); }
	inline QSize sizeHint() const { return QSize(600, 400); }

	void	setNode(const Node* node);
	void	flushGL() const;
	void	saveScreenshot();
    void    setUseLighting(bool do_enable);

protected:

	void initializeGL();
	void resizeGL(int width, int height);
	void paintGL();
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void keyPressEvent(QKeyEvent *event);
	void wheelEvent(QWheelEvent *event);    
};
