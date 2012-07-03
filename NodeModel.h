#pragma once
#include <QAbstractItemModel>
#include <vector>
#include "Node.h"

class NodeModel : public QAbstractItemModel
{
	Q_OBJECT

public:

	explicit NodeModel(QObject *parent, QVector3D geometry, unsigned defaultDilationValue);
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
	void		sortByBBoxSize();
	unsigned	getDefaultDilationValue() const { return _defaultDilationValue; }
	void		setDefaultDilationValue(unsigned defaultDilationValue) { _defaultDilationValue = defaultDilationValue; }

signals:
	void		geometryChanged();
	void		numNodesChanged();
	//void		nodeAdded();
	//void		nodeRemoved(unsigned idx);
public slots:

	Node*		addMesh(const char *filename);
	void		addNode(Node* node);
	void		nodePositionChanged(unsigned i);
	void		clear();

private:

	QVector3D			_geometry;
	std::vector<Node*>	_nodes;
	unsigned			_defaultDilationValue;
};
