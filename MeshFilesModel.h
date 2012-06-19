#pragma once
#include <QAbstractItemModel>
#include "image.h"
#include "Node.h"
#include <vector>

class NodeModel : public QAbstractItemModel
{
	Q_OBJECT

public:

	explicit NodeModel(QObject *parent = 0);
	explicit NodeModel(QVector3D geometry, QObject *parent = 0);
	virtual ~NodeModel();

	Qt::ItemFlags flags(const QModelIndex & index) const;
	int	columnCount( const QModelIndex & parent = QModelIndex()) const;
	int rowCount(const QModelIndex & parent = QModelIndex()) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const;
	QModelIndex parent(const QModelIndex &child) const;
	bool removeRows(int row, int count, const QModelIndex & parent = QModelIndex());
	//const NodeList&	getNodeList() const { return _nodes; }

	Node*		getNode(unsigned i) const { return _nodes[i]; }
	void		setGeometry(QVector3D geometry);
	QVector3D	getGeometry() const { return _geometry; }
	size_t		numNodes() const { return _nodes.size(); }
	void		clear() { _nodes.clear(); }
	void		sortByBBoxSize();

signals:
	void		geometryChanged();
	//void		nodeAdded();
	//void		nodeRemoved(unsigned idx);
public slots:

	Node* addMesh(const char *filename, unsigned dilation);
	void addNode(Node* node);
	void nodePositionChanged(unsigned i);

private:

	QVector3D			_geometry;
	std::vector<Node*>	_nodes;
};
