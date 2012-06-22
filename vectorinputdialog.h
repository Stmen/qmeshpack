#pragma once

#include <QDialog>
#include <QDoubleSpinBox>
#include <QVector3D>

/// A Special dialog, that allowes to input a vector
class VectorInputDialog : public QDialog
{
    Q_OBJECT

	QDoubleSpinBox* _x;
	QDoubleSpinBox* _y;
	QDoubleSpinBox* _z;

public:
	explicit VectorInputDialog(QWidget *parent, QString title, QString text, QVector3D default_input = QVector3D(0., 0., 0.));
	QVector3D getResult() const { return QVector3D(_x->value(), _y->value(), _z->value()); }

signals:
    
public slots:
    
};
