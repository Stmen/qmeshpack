#include <fstream>
#include <algorithm>
#include <sstream>
#include "mesh.h"
#include "Exception.h"
#include "util.h"
#include <cmath>
#include <cassert>
#include <QStringList>
using namespace std;

Mesh::Mesh(const char* off_filename)
{    
    QStringList sl = QString(off_filename).split('/');
    _name = sl.at(sl.size() - 1).split(".").at(0);

    ifstream file;
    file.open(off_filename);

    if (file.fail())
        throw Exception("%s: failed to open file %s", __FUNCTION__, off_filename);

    string line;
    int lineNumber = 0;

    // skip comments
    do
    {
        getline(file, line);
        lineNumber++;
    }
    while(file.good() and line[0] == '#');

    // is signature correct?
    line = line.substr(0, 3);
    if(not file.good() or (line != "OFF" and line != "off"))
        throw Exception("%s: not an OFF file ", __FUNCTION__);
    lineNumber++;

    // skip comments
    do
    {
        getline(file, line);
        lineNumber++;
    }
    while(file.good() and line[0] == '#');

    istringstream iss;
    iss.exceptions((std::_Ios_Iostate)(ifstream::failbit | ifstream::badbit));
    iss >> skipws;
    iss.clear();

    // get vertex_count face_count edge_count
    size_t vertex_count, face_count, edge_count;
    iss.str(line);
    if (not ((iss >> vertex_count) and (iss >> face_count) and (iss >> edge_count)))
        throw Exception("%s: in %s:%u failed to read vertex_count face_count edge_count.", __FUNCTION__, off_filename, lineNumber);
    lineNumber++;

    resetMinMax();

    // process vertices
    for(size_t i = 0; i < vertex_count and file.good(); i++)
    {
        getline(file, line);
        iss.str(line);

        float coord[3];
        for(unsigned i = 0; i < 3; i++)
        {
            if (!(iss >> coord[i]))
                throw Exception("%s: %s:%u failed to parse a vertex.", __FUNCTION__, off_filename, lineNumber);
        }

		_vertices.push_back(QVector3D(coord[0], coord[1], coord[2]));

        lineNumber++;
    }

    // process faces
    for(size_t i = 0; i < face_count and file.good(); i++)
    {
        getline(file, line);
        iss.str(line);
        int poly_type;
        if(iss >> poly_type)
        {
            if(poly_type != 3)
                throw Exception("%s: in %s:%u polygon is not a triangle.", __FUNCTION__, off_filename, lineNumber);

            for(int i = 0; i < poly_type; i++)
            {
                unsigned vertexIndex;
                if(iss >> vertexIndex)
                    _triangles.push_back(vertexIndex);
                else
                    throw Exception("%s: in %s:%u polygon is not a Triangle.", __FUNCTION__, off_filename, lineNumber);
            }
        }
        else
            throw Exception("%s: in %s:%u failed to read number of vertices.", __FUNCTION__, off_filename, lineNumber);

        lineNumber++;
    }
    file.close();
	recalcMinMax();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void Mesh::setVertex(unsigned idx, QVector3D newVertex)
{
    if (idx >= numVertices())
        throw Exception("%s: bad index", __FUNCTION__);

	_vertices[idx] = newVertex;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
QVector3D Mesh::getVertex(unsigned idx) const
{
    if (idx >= numVertices())
        throw Exception("%s: bad index", __FUNCTION__);

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

	for (unsigned i = 0; i < _triangles.size(); i++)
	{
		_min = vecmin(_min, _vertices[_triangles[i]]);
		_max = vecmax(_max, _vertices[_triangles[i]]);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
///
///
///
/////////////////////////////////////////////////////////////////////////////////////////////////////
bool Mesh::Iterator::is_good() const
{
	return (_curr * Triangle::NUM_VERTICES) < _mesh._triangles.size();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
Triangle Mesh::Iterator::get() const
{

	const unsigned* indices = &_mesh._triangles[_curr * Triangle::NUM_VERTICES];
    // the next 3 indices are vertices.
	//cout << "indices: " << indices[0] << " " << indices[1] << " " << indices[2] << " vertices: ";

    Triangle t;
	for (unsigned i = 0; i < Triangle::NUM_VERTICES; i++)
    {        
        t.vertex[i] = _mesh.getVertex(indices[i]);		
    }
	//cout << endl;
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
