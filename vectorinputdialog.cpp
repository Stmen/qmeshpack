#include "vectorinputdialog.h"
#include <QGridLayout>
#include <QDialogButtonBox>

/////////////////////////////////////////////////////////////////////////////////////////////////////
VectorInputDialog::VectorInputDialog(QWidget *parent, QString title, QVector3D default_input) :
    QDialog(parent)
{
	_x = new QDoubleSpinBox(this);
	_y = new QDoubleSpinBox(this);
	_z = new QDoubleSpinBox(this);
	_x->setRange(-1000000, 1000000);
	_y->setRange(-1000000, 1000000);
	_z->setRange(-1000000, 1000000);
	_x->setValue(default_input.x());
	_y->setValue(default_input.y());
	_z->setValue(default_input.z());



	QGridLayout* layout = new QGridLayout(this);

	layout->addWidget(_x, 0 ,0);
	layout->addWidget(_y, 0 ,1);
	layout->addWidget(_z, 0 ,2);
	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
	layout->addWidget(buttonBox, 1, 0, 1, 3);
	setLayout(layout);

	connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
	connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

	setWindowTitle(title);
}
