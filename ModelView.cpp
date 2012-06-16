#include "ModelView.h"
#include <QGridLayout>

ModelView::ModelView(QWidget *parent) :
	QWidget(parent)
{
	_labelTop = new QLabel();
	_labelTop->setBackgroundRole(QPalette::Base);
	//_labelTop->setSizePolicy(QSizePolicy:: Ignored, QSizePolicy::Ignored);
	//_labelTop->setScaledContents(false);
	_labelTop->setScaledContents(true);


	_labelBottom = new QLabel();
	_labelBottom->setBackgroundRole(QPalette::Base);
	//_labelBottom->setSizePolicy(QSizePolicy:: Ignored, QSizePolicy::Ignored);
	//_labelBottom->setScaledContents(false);
	_labelBottom->setScaledContents(true);


	_glView = new GLView();

	QGridLayout* layout = new QGridLayout;
	setLayout(layout);

	layout->addWidget(_labelTop, 0, 0);
	layout->addWidget(_labelBottom, 1, 0);
	layout->addWidget(_glView, 0, 1, 2, 1);
	setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
}

void ModelView::setNode(Node *node, QVector3D geometry)
{
	_labelTop->setPixmap(QPixmap::fromImage(node->getTop()->toQImage(), Qt::ThresholdDither));
	_labelTop->setObjectName(node->getTop()->getName());

	_labelBottom->setPixmap(QPixmap::fromImage(node->getBottom()->toQImage(), Qt::ThresholdDither));
	_labelBottom->setObjectName(node->getBottom()->getName());

	_glView->setNode(node, geometry);
}
