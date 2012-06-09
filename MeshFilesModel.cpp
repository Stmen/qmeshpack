#include "MeshFilesModel.h"

MeshFilesModel::MeshFilesModel(QObject *parent) :
	QAbstractItemModel(parent)
{
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
MeshFilesModel::~MeshFilesModel()
{
	for (unsigned i = 0; i < _meshes.size(); i++)
	{
		delete _meshes[i];
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
int	MeshFilesModel::columnCount(const QModelIndex& parent) const
{
	(void)parent.row(); // supress unused warning
	return 8; // name, w, h, d, top_min_color/max_color, bottom min/max
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
int MeshFilesModel::rowCount(const QModelIndex& parent)const
{
	if (not parent.isValid())
		return _meshes.size();
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
					return QString("name");
				case 1:
					return QString("width");
				case 2:
					return QString("height");
				case 3:
					return QString("depth");
				case 4:
					return QString("bottom min");
				case 5:
					return QString("bottom max");
				case 6:
					return QString("top min");
				case 7:
					return QString("top max");

			}
		}
	}
	return QVariant();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
QVariant MeshFilesModel::data(const QModelIndex& index, int role) const
{
	if (index.isValid() and index.row() >= 0 and (size_t)index.row() < _meshes.size())
	{
		switch (role)
		{
			case Qt::DisplayRole:
				switch(index.column())
				{
					default:
					case 0:
						return _meshes[index.row()]->getMesh()->getName();
					case 1:
						return _meshes[index.row()]->getMesh()->getGeometry().x();
					case 2:
						return _meshes[index.row()]->getMesh()->getGeometry().y();
					case 3:
						return _meshes[index.row()]->getMesh()->getGeometry().z();
					case 4:
						return _meshes[index.row()]->getBottom()->minColor();
					case 5:
						return _meshes[index.row()]->getBottom()->maxColor();
					case 6:
						return _meshes[index.row()]->getTop()->minColor();
					case 7:
						return _meshes[index.row()]->getTop()->maxColor();
				}

			case Qt::UserRole:
			   return qVariantFromValue((void *) _meshes[index.row()]);
			   //return QVariant(QVariant::UserType, _meshes[index.row()]);

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
	if ((not parent.isValid()) and row >= 0 and (size_t)row < _meshes.size() and count)
	{
		beginRemoveRows(parent, row, row + count);
		_meshes.erase(_meshes.begin() + row, _meshes.begin() + row + count);
		endRemoveRows();
		return true;
	}
	else
		return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MeshFilesModel::addMesh(const char* filename, unsigned samples_per_pixel)
{
	RenderedMesh* meshInfo = new RenderedMesh(filename, samples_per_pixel);		
	beginInsertRows(QModelIndex(), _meshes.size(), _meshes.size() + 1);
	_meshes.push_back(meshInfo);
	endInsertRows();
}
