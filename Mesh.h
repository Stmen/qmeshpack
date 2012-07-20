#pragma once
#include <QString>
#include <QObject>
#include <vector>
#include <functional>
#include "util.h"

struct Triangle
{
	QVector3D vertex[3]; /// vertices of this triangle.
	static const unsigned NUM_VERTICES = 3; /// triangle has 3 vertices (at least in this universe)
};

/// this class represents 3D Mesh.
class Mesh
{    
public:

	Mesh(const Mesh& other); /// copy constructor
    Mesh(const char* off_filename); /// OFF mesh constructor.
    ~Mesh();
	void		add(const Mesh& other, const QVector3D offset); /// accumulation of meshes.
	QVector3D	getMax() const { return _max; }
	QVector3D   getMin() const { return _min; }
	QVector3D	getGeometry() const { return _max - _min; } /// mesh BBox
	size_t		numVertices() const { return _vertices.size(); }
	QVector3D   getVertex(unsigned idx) const;
	void		setVertex(unsigned idx, QVector3D newVertex);
	void		resetMinMax(); /// recalculates minimum and maximum coordinates.
	void		scale(const QVector3D factor); /// scales this mesh by some factor.
	void		translate(const QVector3D offset); /// translates all vertices in this mesh by some offset
	QString		getName() const { return _name; }
	void		setName(QString name) { _name = name; }
	QString		getFilename() const { return _filename; }
	void		recalcMinMax(); /// reexamines all vertices and determines new minimum and maximum values.
    void		draw(bool use_lighting) const;
	void		buildNormals();
	void		save(QString filename);
    bool        hasNormals() const { return _normals; }
	double		aabbVolume() const;
	bool		wasFullyTriangulated() const { return _fullyTriangulated; }
	static Mesh*	random(unsigned max_vertices = 5);

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

	Mesh();

	std::vector<QVector3D>	_vertices;  /// this array stores vertix triples of floats, which represent the vertices
	QVector3D*				_normals;  /// this array stores vertix triples of floats, which represent the vertices
	std::vector<unsigned>   _triangleIndices; /// this array hold index triples of the triangles.
	QVector3D               _min; /// minimum x, y, z in this Mesh
	QVector3D               _max; /// maximum x, y, z in this Mesh
	QString                 _name;
	QString					_filename; /// filename, that is the source of this mesh. It is empty if this is an aggregate.
	bool					_fullyTriangulated;
};
