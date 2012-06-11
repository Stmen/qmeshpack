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

Mesh::Mesh(const char* off_filename) :
	_min(INFINITY, INFINITY, INFINITY),
	_max(-INFINITY, -INFINITY, -INFINITY)
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
				{
					_min = vecmin(_min, _vertices[vertexIndex]);
					_max = vecmax(_max, _vertices[vertexIndex]);
					_triangleVertices.push_back(vertexIndex);
				}
                else
                    throw Exception("%s: in %s:%u polygon is not a Triangle.", __FUNCTION__, off_filename, lineNumber);
            }			
        }
        else
            throw Exception("%s: in %s:%u failed to read number of vertices.", __FUNCTION__, off_filename, lineNumber);

        lineNumber++;
    }
    file.close();
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

	for (unsigned i = 0; i < _triangleVertices.size(); i++)
	{
		_min = vecmin(_min, _vertices[_triangleVertices[i]]);
		_max = vecmax(_max, _vertices[_triangleVertices[i]]);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
///
///
///
/////////////////////////////////////////////////////////////////////////////////////////////////////
bool Mesh::Iterator::is_good() const
{
	return (_curr * Triangle::NUM_VERTICES) < _mesh._triangleVertices.size();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
Triangle Mesh::Iterator::get() const
{
	const unsigned* indices = &_mesh._triangleVertices[_curr * Triangle::NUM_VERTICES];

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

	static unsigned indices[] =
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

void Mesh::draw(bool drawAABB) const
{	
	glEnableClientState(GL_VERTEX_ARRAY);
	if (drawAABB)
		drawAxisAlignedBox(_min, _max);
	//glVertexPointer(/* num components */ 3, GL_FLOAT, sizeof(QVector3D), &_vertices[0]);
	glVertexPointer(3, GL_FLOAT, 0, &_vertices[0]);
	glDrawElements(GL_TRIANGLES, _triangleVertices.size(), GL_UNSIGNED_INT, &_triangleVertices[0]);

	glDisableClientState(GL_VERTEX_ARRAY);
}
