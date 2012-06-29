#include <QString>
#include <QStringList>
#include <QFile>
#include <QTextStream>
#include <algorithm>
#include <cmath>
#include <cassert>
#include <map>
#include <vector>
#include "config.h"
#include "mesh.h"
#include "Exception.h"
#include "util.h"


/////////////////////////////////////////////////////////////////////////////////////////////////////
Mesh::Mesh(const Mesh& other) :
	_vertices(other._vertices.size()),
	_normals(0),
	_triangleIndices(other._triangleIndices.size()),
	_min(other._min),
	_max(other._max),
	_name(other._name),
	_filename(other._filename)
{
	memcpy(&_vertices[0], &other._vertices[0], _vertices.size() * sizeof(_vertices[0]));
	if (other._normals)
	{
		_normals = new QVector3D[_vertices.size()];
		memcpy(_normals, other._normals, _vertices.size() * sizeof(QVector3D));
	}
	memcpy(&_triangleIndices[0], &other._triangleIndices[0], _triangleIndices.size() * sizeof(_triangleIndices[0]));
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
Mesh::Mesh(const char* off_filename) :
	_normals(0),
	_min(INFINITY, INFINITY, INFINITY),
	_max(-INFINITY, -INFINITY, -INFINITY)
{    
	_filename = off_filename;

    {
        QStringList sl = _filename.split('/');
        _name = sl.at(sl.size() - 1).split(".").at(0);
    }

    QFile file(_filename);
    if (not file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
		THROW(MeshException, QString("Unable to open file \'%1\' for reading.").arg(file.errorString()));

    }

    QTextStream in(&file);

    int lineNumber = 0;
    QString line;

    do
    {
        line = in.readLine();
        lineNumber++;
    }
    while (not line.isEmpty() and line.at(0) == '#');


    // is signature correct?
    line.truncate(3);
    if(line.isNull() or (line != "OFF" and line != "off"))
		THROW(MeshException, QString("\'%1\'' is not an OFF file ").arg(off_filename));
    lineNumber++;

    // skip comments
    do
    {
        line = in.readLine();
        lineNumber++;
    }
    while(not line.isNull() and line.at(0) == '#');
    QStringList lst = line.split(' ');

    // get vertex_count face_count edge_count
	size_t vertex_count = lst.at(0).toULong(), face_count = lst.at(1).toULong();//, edge_count = lst.at(2).toULong();

    resetMinMax();

    // process vertices
    for (size_t i = 0; i < vertex_count and not in.atEnd(); i++)
    {
        lineNumber++;
        float coord[3];
        in >> coord[0] >> coord[1] >> coord[2];
        if (in.status() != QTextStream::Ok)
			THROW(MeshException, QString(" in %1:%1 failed to read coordinate.").arg(off_filename, QString::number(lineNumber)));
		_vertices.push_back(QVector3D(coord[0], coord[1], coord[2]));
    }

    // process faces
    for(size_t i = 0; i < face_count and not in.atEnd(); i++)
    {
        unsigned firstIndex, lastIndex;
        int poly_type;
        in >> poly_type;
        if (in.status() == QTextStream::Ok)
        {
            /*
            if(poly_type != 3)
                throw Exception("%s: in %s:%u polygon is not a triangle.", __FUNCTION__, off_filename, lineNumber);
            */

            unsigned vertexIndex;
            for(int i = 0; i < poly_type; i++)
            {
                lastIndex = vertexIndex;
                in >> vertexIndex;
                if (in.status() == QTextStream::Ok)
				{
                    if (i == 0)
                        firstIndex = vertexIndex;
                    else if (i > 2) // this triangulation should work for convex polygons
                    {
                        _triangleIndices.push_back(firstIndex);
                        _triangleIndices.push_back(lastIndex);
                    }

					_min = vecmin(_min, _vertices[vertexIndex]);
					_max = vecmax(_max, _vertices[vertexIndex]);
					_triangleIndices.push_back(vertexIndex);
				}
                else
					THROW(MeshException, QString("in %1:%2 polygon is not a Triangle.").arg(off_filename, QString::number(lineNumber)));
            }			
        }
        else
			THROW(MeshException, QString("in %1:%2 failed to read number of vertices.").arg(off_filename, QString::number(lineNumber)));

        lineNumber++;
    }
    file.close();

#ifdef USE_LIGHTING
	buildNormals();
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
Mesh::~Mesh()
{
	if (_normals)
		delete [] _normals;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void Mesh::add(const Mesh& other, const QVector3D offset)
{
	_filename.clear();

	size_t oldVertSize = _vertices.size();
	_vertices.resize(oldVertSize + other._vertices.size());
	for (unsigned i = 0; i < other._vertices.size(); i++)
		_vertices[oldVertSize + i] = other._vertices[i] + offset;

	if (_normals)
	{
		QVector3D* old_normals = _normals;
		_normals = new QVector3D[_vertices.size()];
		memcpy(_normals, old_normals, oldVertSize * sizeof(QVector3D));
		if (other._normals)
			memcpy(&_normals[oldVertSize], other._normals, other._vertices.size() * sizeof(QVector3D));
		else
			memset(&_normals[oldVertSize], 0, other._vertices.size() * sizeof(QVector3D));
		delete [] old_normals;
	}

	size_t oldTriSize = _triangleIndices.size();
	_triangleIndices.resize(oldTriSize + other._triangleIndices.size());
	for (unsigned i = 0; i < other._triangleIndices.size(); i++)
		_triangleIndices[oldTriSize + i] = other._triangleIndices[i] + oldVertSize; // adjusting the indices.

	_min = vecmin(_min, other._min);
	_max = vecmax(_max, other._max);
	_name = QString("%1+%2").arg(_name).arg(other._name);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void Mesh::save(QString filename)
{
	QFile file(filename);
	if (not file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		THROW(MeshException, QString("Unable to open file \'%1\' for writing.").arg(file.errorString()));
		return;
	}	

	if (filename.endsWith(".off") or filename.endsWith(".OFF"))
	{
		QTextStream out(&file);
		out << "OFF\n";
		out << _vertices.size() << ' ' << (_triangleIndices.size() / 3) << " 0\n";

		for (unsigned i = 0; i < _vertices.size(); i++)
			out << _vertices[i].x() << ' ' <<  _vertices[i].y() << ' ' <<  _vertices[i].z() << '\n';

		for (unsigned i = 0; i < _triangleIndices.size(); i += 3)
			out << "3 " << _triangleIndices[i] << ' ' << _triangleIndices[i + 1] << ' ' <<  _triangleIndices[i + 2] << '\n';

	}
	else if (filename.endsWith(".stl") or filename.endsWith(".STL"))
	{
		if (not _normals)
			buildNormals();

		QString name = _name.isEmpty() ? "NamelessSolid" : _name;
		QTextStream out(&file);
		out.setRealNumberNotation(QTextStream::ScientificNotation);
		out << "solid " << name << '\n';
		for (unsigned i = 0; i < _triangleIndices.size(); i += 3)
		{
			const unsigned* indices = &_triangleIndices[i];
			QVector3D normal = (_normals[indices[0]] + _normals[indices[1]] + _normals[indices[2]]).normalized();
			out << "facet normal " << normal.x() << ' ' << normal.y() << ' ' << normal.z() << '\n';
			out << "outer loop\n";

			for (unsigned j = 0; j < 3; j++)
			{
				QVector3D vertex = _vertices[indices[j]];
				out << "vertex " << vertex.x() << ' ' << vertex.y() << ' ' << vertex.z() << '\n';

			}

			out << "endloop\n"
				   "endfacet\n";
		}
		out << "endsolid " << name << '\n';
	}
	else
		THROW(MeshException, QString("unknown mesh extension in \'%s.\'").arg(filename));

	file.close();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void Mesh::setVertex(unsigned idx, QVector3D newVertex)
{
    if (idx >= numVertices())
		THROW(MeshException, QString("bad index %1").arg(QString::number(idx)));

	_vertices[idx] = newVertex;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
QVector3D Mesh::getVertex(unsigned idx) const
{
    if (idx >= numVertices())
		THROW(MeshException, QString("bad index %1").arg(QString::number(idx)));

	return _vertices[idx];
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
///
/// clear min/max
///
/////////////////////////////////////////////////////////////////////////////////////////////////////
void Mesh::resetMinMax()
{
	_min = QVector3D(INFINITY, INFINITY, INFINITY);
	_max = QVector3D(-INFINITY, -INFINITY, -INFINITY);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
///
/// scales the mesh
///
/////////////////////////////////////////////////////////////////////////////////////////////////////
void Mesh::scale(const QVector3D factor)
{
    _max *= factor;
    _min *= factor;

    for (size_t i = 0; i < numVertices(); i ++)
		_vertices[i] *= factor;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
///
/// translates this mesh
///
/////////////////////////////////////////////////////////////////////////////////////////////////////
void Mesh::translate(const QVector3D offset)
{
    _max += offset;
    _min += offset;

	for (size_t i = 0; i < _vertices.size(); i ++)
		_vertices[i] += offset;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
///
/// Recalculates minimum/maximum values of a mesh.
///
/////////////////////////////////////////////////////////////////////////////////////////////////////
void Mesh::recalcMinMax()
{
	_min = QVector3D(INFINITY, INFINITY, INFINITY);
	_max = QVector3D(-INFINITY, -INFINITY, -INFINITY);

	for (unsigned i = 0; i < _triangleIndices.size(); i++)
	{
		_min = vecmin(_min, _vertices[_triangleIndices[i]]);
		_max = vecmax(_max, _vertices[_triangleIndices[i]]);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void Mesh::buildNormals()
{
	typedef QMap</* index = */unsigned, /* normals = */ std::vector<QVector3D>*> Normals;
    Normals normals;

	for (unsigned i = 0; i < _triangleIndices.size(); i += 3)
	{
		unsigned* indices = &_triangleIndices[i];
		QVector3D vertex[3] =
		{
			_vertices[indices[0]],
			_vertices[indices[1]],
			_vertices[indices[2]]
		};

		QVector3D normal = QVector3D::crossProduct(
							   vertex[1] - vertex[0],
							   vertex[2] - vertex[0]).normalized();

        for (unsigned j = 0; j < 3; j++)
        {
			Normals::iterator it = normals.find(indices[j]);
            if (it == normals.end())
            {
				it = normals.insert(indices[j], new std::vector<QVector3D>);
            }
			it.value()->push_back(normal);
        }
	}

	if (_normals)
		delete [] _normals;
	_normals = new QVector3D[_vertices.size()];

    for (Normals::iterator it = normals.begin(); it != normals.end(); ++it)
    {
		std::vector<QVector3D>* vec = it.value();
        QVector3D normal(0., 0., 0.);
        for (unsigned i = 0; i < vec->size(); i++)
            normal += vec->at(i);

        normal.normalize();
		unsigned idx = it.key();
		_normals[idx] = normal;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
///
///
///
/////////////////////////////////////////////////////////////////////////////////////////////////////
bool Mesh::Iterator::is_good() const
{
	return (_curr * Triangle::NUM_VERTICES) < _mesh._triangleIndices.size();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
Triangle Mesh::Iterator::get() const
{
	const unsigned* indices = &_mesh._triangleIndices[_curr * Triangle::NUM_VERTICES];

    Triangle t;
	for (unsigned i = 0; i < Triangle::NUM_VERTICES; i++)
        t.vertex[i] = _mesh.getVertex(indices[i]);		

    return t;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
///
///
///
/////////////////////////////////////////////////////////////////////////////////////////////////////
void Mesh::Iterator::next()
{
    _curr++;    
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
///
///	Draws this mesh.
///
/////////////////////////////////////////////////////////////////////////////////////////////////////
#include <qgl.h>
void drawAxisAlignedBox(QVector3D min, QVector3D max)
{
	// AABB Vertices
	GLdouble data[] =
	{
		min.x(), min.y(), min.z(),
		min.x(), max.y(), min.z(),
		max.x(), max.y(), min.z(),
		max.x(), min.y(), min.z(),
		min.x(), min.y(), max.z(),
		min.x(), max.y(), max.z(),
		max.x(), max.y(), max.z(),
		max.x(), min.y(), max.z()
	};

	static const unsigned indices[] =
	{
			0, 1, 2, 3, // front face
			0, 4, 5, 1, // left face
			2, 6, 5, // top face
			4, 7, 6, // back face
			2, 3, 7 // right face
	};

	glVertexPointer(3, GL_DOUBLE, 0, data);
	glDrawElements(GL_LINE_STRIP, sizeof(indices) / sizeof(indices[0]), GL_UNSIGNED_INT, indices);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void Mesh::draw(bool drawAABB) const
{	
	//glEnableClientState(GL_VERTEX_ARRAY); // enabling/disabling client state is moved to GLView
	if (drawAABB)
		drawAxisAlignedBox(_min, _max);

#ifdef USE_LIGHTING
	//glEnableClientState(GL_NORMAL_ARRAY);
	glNormalPointer(GL_FLOAT, sizeof(QVector3D), (void*)_normals);
#endif

	//glVertexPointer(/* num components */ 3, GL_FLOAT, sizeof(QVector3D), &_vertices[0]);
	glVertexPointer(3, GL_FLOAT, sizeof(QVector3D), &_vertices[0]);
	glDrawElements(GL_TRIANGLES, _triangleIndices.size(), GL_UNSIGNED_INT, &_triangleIndices[0]);

#ifdef USE_LIGHTING
	//glDisableClientState(GL_NORMAL_ARRAY);
#endif

	//glDisableClientState(GL_VERTEX_ARRAY);

}
