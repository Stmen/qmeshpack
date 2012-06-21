#pragma once

#include <QSplitter>
#include <QLabel>
#include "Node.h"
#include "GLView.h"

class ModelView : public QSplitter
{
	Q_OBJECT

	QLabel*	_labelTop;
	QLabel*	_labelBottom;
	GLView*	_glView;

public:

	explicit ModelView(QWidget *parent = 0);
	GLView*	getGLView() const { return _glView; }	

signals:
	
public slots:

	void	setNode(Node* node);
	void	setScaleImages(bool doScale);
	bool	areImagesScaled() const { return _labelTop->hasScaledContents(); }

};
