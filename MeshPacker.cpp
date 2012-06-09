#include "MeshPacker.h"
#include <cassert>
#include <omp.h>
#include <atomic>
#include <future>

/////////////////////////////////////////////////////////////////////////////////////////////////////
MeshPacker::MeshPacker(QVector3D size, QObject *parent) :
	QThread(parent), _geometry(size), _meshes(0)
{
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MeshPacker::setMeshList(const MeshList* meshes)
{
	delete _meshes;
	_meshes = meshes;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
///
/// Returns maximum number, that the reportProgress() signal will report.
///
/////////////////////////////////////////////////////////////////////////////////////////////////////
size_t MeshPacker::maxProgress() const
{
	assert(_meshes);
	size_t progress = 0;
	for (unsigned i = 0; i < _meshes->size(); i++)
	{
		RenderedMesh* rmesh = _meshes->at(i);
		QVector3D max = _geometry - rmesh->getMesh()->getGeometry();
		progress += max.x() * max.y();
	}
	return progress;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MeshPacker::run()
{
	_results.clear();
	Image base(_geometry.x(), _geometry.y(), /* clear color = */ 0.);
	std::atomic<size_t> progress_atom;
	progress_atom = 0;
	for (size_t i = 0; i < _meshes->size(); i++)
	{
		RenderedMesh* rendered_mesh = _meshes->at(i);
		emit report(QString("processing Mesh \"%1\"").arg(rendered_mesh->getMesh()->getName()));
		QVector3D max = _geometry - rendered_mesh->getMesh()->getGeometry();

		Image::ColorType best_z = base.computeMinZ(0, 0, *(rendered_mesh->getBottom()));
		unsigned best_x = 0;
		unsigned best_y = 0;

		//collapse(2)
		unsigned max_y = max.y();
		unsigned max_x = max.x();

		#pragma omp parallel for collapse(2)
		for (unsigned y = 0; y < max_y; y++)
		{
			for (unsigned x = 0; x < max_x; x++)
			{
				size_t progress = atomic_fetch_add(&progress_atom, (size_t)1);
				emit reportProgress(progress);

				Image::ColorType z = base.computeMinZ(x, y, *(rendered_mesh->getBottom()));
				#pragma omp critical
				{
					if (z < best_z)
					{
						best_z = z;
						best_x = x;
						best_y = y;
					}
				}
			}
		}
		base.insertAt(best_x, best_y, best_z, *(rendered_mesh->getTop()));
		QVector3D result = QVector3D(best_x, best_y, best_z) + rendered_mesh->getMesh()->getMin();
		_results.push_back(result);
	}

	emit processingDone();
}
