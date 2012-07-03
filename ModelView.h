#pragma once
#include <QSplitter>
#include <QLabel>
#include "GLView.h"

class ModelView : public QSplitter
{
	Q_OBJECT

	QLabel*	_labelTop;
	QLabel*	_labelBottom;
	GLView*	_glView;

public:

    explicit		ModelView(bool use_lighting, QWidget *parent = NULL);
	const GLView*	getGLView() const { return _glView; }
    GLView*         getGLView() { return _glView; }

signals:
	
public slots:

	void	setNode(Node* node);
	void	setScaleImages(bool doScale);
	bool	areImagesScaled() const { return _labelTop->hasScaledContents(); }

};
