#pragma once
#include <QAbstractItemModel>
#include "image.h"
#include "Node.h"
#include <vector>

class MeshFilesModel : public QAbstractItemModel
{
	Q_OBJECT

public:

	Node*		getNode(unsigned i) const { return _nodes[i]; }
	void		setGeometry(QVector3D geometry);
	QVector3D	getGeometry() const { return _geometry; }
	size_t		numNodes() const { return _nodes.size(); }

	explicit MeshFilesModel(QVector3D geometry, QObject *parent = 0);
	virtual ~MeshFilesModel();
	Qt::ItemFlags flags(const QModelIndex & index) const;
	int	columnCount( const QModelIndex & parent = QModelIndex()) const;
	int rowCount(const QModelIndex & parent = QModelIndex()) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const;
	QModelIndex parent(const QModelIndex &child) const;
	bool removeRows(int row, int count, const QModelIndex & parent = QModelIndex());
	//const NodeList&	getNodeList() const { return _nodes; }

signals:
	void		geometryChanged();
	//void		nodeAdded();
	//void		nodeRemoved(unsigned idx);
public slots:

	void addMesh(const char *filename, unsigned samples_per_pixel = 10, unsigned dilation = 0);
	void addNode(Node* node);

private:

	QVector3D			_geometry;
	std::vector<Node*>	_nodes;
};
