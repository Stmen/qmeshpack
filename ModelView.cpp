#include "ModelView.h"
#include <QGridLayout>

/////////////////////////////////////////////////////////////////////////////////////////////////////
ModelView::ModelView(QWidget *parent) :
	QSplitter(parent)
{	

	_labelTop = new QLabel();
	_labelTop->setBackgroundRole(QPalette::Base);
	_labelTop->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	_labelTop->setScaledContents(false);

	_labelBottom = new QLabel();
	_labelBottom->setBackgroundRole(QPalette::Base);
	_labelBottom->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	_labelBottom->setScaledContents(false);

	QSplitter* splitter = new QSplitter(this);
	splitter->setOrientation(Qt::Vertical);	
	splitter->addWidget(_labelTop);
	splitter->addWidget(_labelBottom);
	splitter->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	addWidget(splitter);

	_glView = new GLView();
	addWidget(_glView);

	setOrientation(Qt::Horizontal);
	setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void ModelView::setNode(Node *node)
{
	_labelTop->setPixmap(QPixmap::fromImage(node->getTop()->toQImage(), Qt::ThresholdDither));
	_labelTop->setObjectName(node->getTop()->getName());

	_labelBottom->setPixmap(QPixmap::fromImage(node->getBottom()->toQImage(), Qt::ThresholdDither));
	_labelBottom->setObjectName(node->getBottom()->getName());

	_glView->setNode(node);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void ModelView::setScaleImages(bool doScale)
{
	_labelTop->setScaledContents(doScale);
	_labelBottom->setScaledContents(doScale);
}
