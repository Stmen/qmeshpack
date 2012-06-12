#pragma once
#include <QString>
#include <QObject>
#include <vector>
#include <functional>
#include "util.h"

struct Triangle
{
	QVector3D vertex[3]; /// vertices of this triangle.
	static const unsigned NUM_VERTICES = 3;
};

/// this class represents the OFF Mesh.
class Mesh
{    
public:

	Mesh(const char* off_filename);
	QVector3D	getMax() const { return _max; }
	QVector3D   getMin() const { return _min; }
	QVector3D	getGeometry() const { return _max - _min; }
	size_t		numVertices() const { return _vertices.size(); }
	QVector3D   getVertex(unsigned idx) const;
	void		setVertex(unsigned idx, QVector3D newVertex);
	void		resetMinMax(); /// recalculates minimum and maximum coordinates.
	void		scale(const QVector3D factor); /// scales the Mesh by some factors.
	void		translate(const QVector3D offset);
	QString		getName() const { return _name; }
	void		recalcMinMax();
	void		draw(bool drawAABB = true) const;
	void		buildNormals();

    /// this class is used to iterate over the Triangles of a Mesh.
    class Iterator : public ::Iterator<Triangle>
    {
		friend class Mesh;

        const Mesh& _mesh;
        unsigned _curr;

        inline Iterator(const Mesh& mesh) : _mesh(mesh), _curr(0) {}

    public:

        inline virtual ~Iterator() {}
        bool is_good() const; /// returns true if this iterator is valid.
        Triangle get() const; /// returns current triangle.
        void next();
    };

    Iterator vertexIterator() const { return Iterator(*this); }

private:

	std::vector<QVector3D>	_vertices;  /// this array stores vertix triples of floats, which represent the vertices
	std::vector<unsigned>   _triangleIndices; /// this array hold index triples of the triangles.
	QVector3D               _min; /// minimum x, y, z in this Mesh
	QVector3D               _max; /// maximum x, y, z in this Mesh
    QString                 _name;

};

void drawAxisAlignedBox(QVector3D min, QVector3D max);
