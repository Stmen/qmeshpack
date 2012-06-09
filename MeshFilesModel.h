#pragma once
#include <QAbstractItemModel>
#include "image.h"
#include <vector>
#include <RenderedMesh.h>

class MeshFilesModel : public QAbstractItemModel
{
	Q_OBJECT

public:



	explicit MeshFilesModel(QObject *parent = 0);
	virtual ~MeshFilesModel();
	Qt::ItemFlags flags(const QModelIndex & index) const;
	int	columnCount( const QModelIndex & parent = QModelIndex()) const;
	int rowCount(const QModelIndex & parent = QModelIndex()) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const;
	QModelIndex parent(const QModelIndex &child) const;
	bool removeRows(int row, int count, const QModelIndex & parent = QModelIndex());
	const MeshList*	getMeshList() const { return &_meshes; }

signals:

public slots:
	void addMesh(const char *filename, unsigned samples_per_pixel = 10);

private:

	std::vector<RenderedMesh*>  _meshes;
};
