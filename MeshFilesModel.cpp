#include "MeshFilesModel.h"

MeshFilesModel::MeshFilesModel(QVector3D geometry, QObject *parent) :
	QAbstractItemModel(parent), _geometry(geometry)
{
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
MeshFilesModel::~MeshFilesModel()
{
	/*
	for (unsigned i = 0; i < _nodes.size(); i++)
	{
		delete _nodes[i];
	}
	*/
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
int	MeshFilesModel::columnCount(const QModelIndex& parent) const
{
	(void)parent.row(); // supress unused warning
	return 5; // name, position, geometry, dilation value, samples per pixel
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
int MeshFilesModel::rowCount(const QModelIndex& parent)const
{
	if (not parent.isValid())
		return _nodes.size();
	else
		return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
Qt::ItemFlags MeshFilesModel::flags(const QModelIndex &index) const
{
	(void)index.row(); // supress unused warning
	return  Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
QVariant MeshFilesModel::headerData(int section, Qt::Orientation orientation, int role) const
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
				case 4:
					return QString("Samples per pixel");
			}
		}
	}
	return QVariant();
}

QString toString(QVector3D vec)
{
	return QString("(%1, %2, %3)").arg(vec.x()).arg(vec.y()).arg(vec.z());
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
QVariant MeshFilesModel::data(const QModelIndex& index, int role) const
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
					case 4:
						return node->getSamplesPerPixel();
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
QModelIndex MeshFilesModel::index(int row, int column, const QModelIndex& parent) const
{
	if (not parent.isValid())
		return createIndex(row, column, 0);
	else
		return QModelIndex();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
QModelIndex MeshFilesModel::parent(const QModelIndex &child) const
{
	(void)child.row();
	return QModelIndex();
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
bool MeshFilesModel::removeRows(int row, int count, const QModelIndex& parent)
{
	if ((not parent.isValid()) and row >= 0 and (size_t)row < _nodes.size() and count)
	{
		beginRemoveRows(parent, row, row + count);
		_nodes.erase(_nodes.begin() + row, _nodes.begin() + row + count);
		endRemoveRows();
		return true;
	}
	else
		return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MeshFilesModel::addNode(Node* node)
{
	beginInsertRows(QModelIndex(), _nodes.size(), _nodes.size() + 1);
	_nodes.push_back(node);
	endInsertRows();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MeshFilesModel::addMesh(const char* filename, unsigned samples_per_pixel, unsigned dilation)
{
	Node* node = new Node(filename, samples_per_pixel, dilation);
	beginInsertRows(QModelIndex(), _nodes.size(), _nodes.size() + 1);
	_nodes.push_back(node);
	endInsertRows();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MeshFilesModel::setGeometry(QVector3D geometry)
{
	_geometry = geometry;
	emit geometryChanged();
}
