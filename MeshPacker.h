#ifndef MESHPACKER_H
#define MESHPACKER_H
#include <QThread>
#include "image.h"
#include "RenderedMesh.h"

#define PACKER_DEFAULT_GEOMETRY QVector3D(1000., 1000., 1000.)

class MeshPacker : public QThread
{
	Q_OBJECT

	QVector3D			_geometry;
	const MeshList*		_meshes;
	std::vector<QVector3D> _results;

public:
	explicit MeshPacker(QVector3D size, QObject *parent = 0);

	size_t numMeshes() const { return _meshes->size(); }
	void setGeometry(const QVector3D geometry) { _geometry = geometry; }
	void setMeshList(const MeshList *meshes);
	QVector3D getGeometry() const { return _geometry; }
	const std::vector<QVector3D>&	getResults() const { return _results; }
	size_t maxProgress() const;
protected:
	void run();

signals:

	void reportProgress(int progress);
	void processingDone();
	void report(QString what);

public slots:
	
};

#endif // MESHPACKER_H
