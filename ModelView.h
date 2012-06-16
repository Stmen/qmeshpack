#ifndef MODELVIEW_H
#define MODELVIEW_H

#include <QWidget>
#include <QLabel>
#include "Node.h"
#include "GLView.h"

class ModelView : public QWidget
{
	Q_OBJECT

	QLabel*	_labelTop;
	QLabel*	_labelBottom;
	GLView*	_glView;


public:
	explicit ModelView(QWidget *parent = 0);
	
signals:
	
public slots:

	void setNode(Node* node, QVector3D geometry);
};

#endif // MODELVIEW_H
