#include "ModelView.h"
#include <QGridLayout>

/////////////////////////////////////////////////////////////////////////////////////////////////////
ModelView::ModelView(QWidget *parent) :
	QSplitter(parent),
	_uiScaleImages(false)
{	

	_labelTop = new QLabel();
	_labelTop->setBackgroundRole(QPalette::Base);
	_labelTop->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	_labelTop->setScaledContents(_uiScaleImages);

	_labelBottom = new QLabel();
	_labelBottom->setBackgroundRole(QPalette::Base);
	_labelBottom->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	_labelBottom->setScaledContents(_uiScaleImages);

	setOrientation(Qt::Horizontal);
	QSplitter* splitter = new QSplitter(this);
	splitter->setOrientation(Qt::Vertical);
	splitter->addWidget(_labelTop);

	splitter->addWidget(_labelBottom);
	addWidget(splitter);

	_glView = new GLView();
	addWidget(_glView);

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
