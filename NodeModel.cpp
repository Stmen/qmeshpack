#include <QtConcurrent/QtConcurrentMap>
#include <functional>
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
	return 5; // name, position, dilation value, AABB size, AABB volume
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
	Qt::ItemFlags flags = 0;
	if (index.column() == 0 || index.column() == 2)
		flags |= Qt::ItemIsEditable;

	flags |= Qt::ItemIsSelectable | Qt::ItemIsEnabled;
	return flags;
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
				case Name:
					return QString("Name");
				case Position:
					return QString("Position");
				case Dilation:
					return QString("Dilation value");
				case AABBSize:
					return QString("AABB Size");
				case AABBVolume:
					return QString("AABB Volume");
			}
		}
	}
	return QVariant();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
bool NodeModel::setData(const QModelIndex & index, const QVariant& value, int role)
{
	bool ok = false;
	if (role == Qt::EditRole and index.isValid() and index.row() >= 0 and (size_t)index.row() < _nodes.size())
	{
		if (index.column() == (int)Name)
		{
			emit dataChanged(index, index);
			_nodes[index.row()]->getMesh()->setName(value.toString());
			return true;
		}
		else if (index.column() == (int)Dilation)
		{
			int dval = value.toUInt(&ok);
			if (ok)
			{
				_nodes[index.row()]->setDilationValue(dval);
				emit dataChanged(index, index);
			}
		}
	}

	return false;
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
					case Name:
                        return node->getMesh()->getName();
					case Position:
						return toString(node->getPos());
					case Dilation:
						return node->getDilationValue();
					case AABBSize:
						return toString(node->getMesh()->getGeometry());
					case AABBVolume:
					{
						QVector3D geom = node->getMesh()->getGeometry();
						return geom.x() * geom.y() * geom.z();
					}
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
    {
        return createIndex(row, column);
    }
	else
    {
		return QModelIndex();
    }
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
static bool greaterVolume(const Node* n1, const Node* n2)
{
    QVector3D g1 = n1->getMesh()->getGeometry();
    QVector3D g2 = n2->getMesh()->getGeometry();

    return (g1.x() * g1.y() * g1.z()) > (g2.x() * g2.y() * g2.z());
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
static bool greaterHeight(const Node* n1, const Node* n2)
{
	QVector3D g1 = n1->getMesh()->getGeometry();
	QVector3D g2 = n2->getMesh()->getGeometry();

	return (g1.y() > g2.y());
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
static bool greaterTopBottomVolume(const Node* n1, const Node* n2)
{
	double vol1 = n1->getTop()->diffSum(*n1->getBottom());
	double vol2 = n2->getTop()->diffSum(*n2->getBottom());
	return (vol1 > vol2);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void NodeModel::sort(int column, Qt::SortOrder order)
{
	std::function<bool (const Node*, const Node*)> comparator;
	switch((ColumnIndex)column)
	{
		case Name:
			if (order == Qt::AscendingOrder)
				comparator = [](const Node* n1, const Node* n2){ return n1->getMesh()->getName() < n2->getMesh()->getName(); };
			else
				comparator = [](const Node* n1, const Node* n2){ return n1->getMesh()->getName() > n2->getMesh()->getName(); };
			break;

		case Position:
			if (order == Qt::AscendingOrder)
				comparator = [](const Node* n1, const Node* n2){ return n1->getPos() < n2->getPos(); };
			else
				comparator = [](const Node* n1, const Node* n2){ return n1->getPos() > n2->getPos(); };
			break;

		case Dilation:
			if (order == Qt::AscendingOrder)
				comparator = [](const Node* n1, const Node* n2){ return n1->getDilationValue() < n2->getDilationValue(); };
			else
				comparator = [](const Node* n1, const Node* n2){ return n1->getDilationValue() > n2->getDilationValue(); };
			break;

		case AABBSize:
			if (order == Qt::AscendingOrder)
				comparator = [](const Node* n1, const Node* n2){ return n1->getMesh()->getGeometry() < n2->getMesh()->getGeometry(); };
			else
				comparator = [](const Node* n1, const Node* n2){ return n1->getMesh()->getGeometry() > n2->getMesh()->getGeometry(); };
			break;

		case AABBVolume: // AABB Volume
			if (order == Qt::AscendingOrder)
				comparator = [](const Node* n1, const Node* n2){ return n1->getAABBVolume() < n2->getAABBVolume(); };
			else
				comparator = [](const Node* n1, const Node* n2){ return n1->getAABBVolume() > n2->getAABBVolume(); };
			break;

		default:
			return;
	}

	beginResetModel();
	std::sort(_nodes.begin(), _nodes.end(), comparator);
	endResetModel();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void NodeModel::sortByBBoxSize()
{
	beginResetModel();
	bool (*comparator)(const Node*, const Node*);

#if MODELSORT == 1
	comparator = greaterVolume;
#elif MODELSORT == 2
	comparator == greaterHeight;
#else
	comparator = greaterVolume;
#endif


	std::sort(_nodes.begin(), _nodes.end(), comparator);
	//emit dataChanged(createIndex(0, 0), createIndex(_nodes.size() - 1, 0););
	endResetModel();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
double NodeModel::nodesVolume() const
{
	double volume = 0.;
	for (unsigned i = 0; i < _nodes.size(); i++)
	{
		volume += _nodes[i]->getMesh()->aabbVolume();
	}
	return volume;
}

