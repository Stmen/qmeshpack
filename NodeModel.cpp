#include "NodeModel.h"
#include "util.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////
NodeModel::NodeModel(QObject *parent, QVector3D geometry, unsigned defaultDilationValue) :
	QAbstractItemModel(parent), _geometry(geometry), _defaultDilationValue(defaultDilationValue)
{
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
NodeModel::~NodeModel()
{
	clear();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
int	NodeModel::columnCount(const QModelIndex& parent) const
{
	(void)parent.row(); // supress unused warning
	return 4; // name, position, geometry, dilation value
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
int NodeModel::rowCount(const QModelIndex& parent)const
{
	if (not parent.isValid())
		return _nodes.size();
	else
		return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
Qt::ItemFlags NodeModel::flags(const QModelIndex &index) const
{
	(void)index.row(); // supress unused warning
	return  Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
QVariant NodeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role == Qt::DisplayRole)
	{
		if (orientation == Qt::Horizontal)
		{
			switch (section)
			{
				default:
				case 0:
					return QString("Name");
				case 1:
					return QString("Poisition");
				case 2:
					return QString("Geometry");
				case 3:
					return QString("Dilation value");
			}
		}
	}
	return QVariant();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
QVariant NodeModel::data(const QModelIndex& index, int role) const
{
	if (index.isValid() and index.row() >= 0 and (size_t)index.row() < _nodes.size())
	{
		Node* node = _nodes[index.row()];
		switch (role)
		{
			case Qt::DisplayRole:				
				switch(index.column())
				{
					default:
					case 0:
						return node->getMesh()->getName();
					case 1:
						return toString(node->getPos());
					case 2:
						return toString(node->getMesh()->getGeometry());
					case 3:
						return node->getDilationValue();
				}

			case Qt::UserRole:
			   return qVariantFromValue((void *)node);

			default:
				return QVariant();
		}

	}

	return QVariant();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
QModelIndex NodeModel::index(int row, int column, const QModelIndex& parent) const
{
	if (not parent.isValid())
		return createIndex(row, column, 0);
	else
		return QModelIndex();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
QModelIndex NodeModel::parent(const QModelIndex &child) const
{
	(void)child.row();
	return QModelIndex();
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
bool NodeModel::removeRows(int row, int count, const QModelIndex& parent)
{
	if ((not parent.isValid()) and row >= 0 and (size_t)row < _nodes.size() and count)
	{
		beginRemoveRows(parent, row, row + count);
		_nodes.erase(_nodes.begin() + row, _nodes.begin() + row + count);
		endRemoveRows();
		emit numNodesChanged();
		return true;
	}
	else
		return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void NodeModel::addNode(Node* node)
{
	beginInsertRows(QModelIndex(), _nodes.size(), _nodes.size() + 1);
	_nodes.push_back(node);
	endInsertRows();
	emit numNodesChanged();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
Node* NodeModel::addMesh(const char* filename)
{
	Node* node = new Node(filename, _defaultDilationValue);
	beginInsertRows(QModelIndex(), _nodes.size(), _nodes.size() + 1);
	_nodes.push_back(node);
	endInsertRows();
	emit numNodesChanged();
	return node;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void NodeModel::clear()
{
	size_t numNodes = _nodes.size();
	beginRemoveRows(QModelIndex(), 0, numNodes - 1);
	for (unsigned i = 0; i < numNodes; i++)
	{
		Node* node = _nodes.back();
		_nodes.pop_back();
		delete node;
	}
	endRemoveRows();
	emit numNodesChanged();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void NodeModel::setGeometry(QVector3D geometry)
{
	_geometry = geometry;
	emit geometryChanged();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void NodeModel::nodePositionChanged(unsigned i)
{
	if (i < _nodes.size())
	{
        QModelIndex idx = createIndex(i, /* columnt with position = */ 1);
        emit dataChanged(idx, idx);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
static bool greaterVolume(Node* n1, Node* n2)
{
	QVector3D g1 = n1->getMesh()->getGeometry();
	QVector3D g2 = n2->getMesh()->getGeometry();

    return (g1.x() * g1.y() * g1.z()) > (g2.x() * g2.y() * g2.z());
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
void NodeModel::sortByBBoxSize()
{
	beginResetModel();
    std::sort(_nodes.begin(), _nodes.end(), greaterVolume);
	//emit dataChanged(createIndex(0, 0), createIndex(_nodes.size() - 1, 0););
	endResetModel();
}
