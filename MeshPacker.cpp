#include "MeshPacker.h"
#include <cassert>
#include <omp.h>
#include <atomic>
#include <future>

/////////////////////////////////////////////////////////////////////////////////////////////////////
MeshPacker::MeshPacker(MeshFilesModel &nodes, QObject *parent) :
	QThread(parent), _nodes(nodes)
{
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
///
/// Returns maximum number, that the reportProgress() signal will report.
///
/////////////////////////////////////////////////////////////////////////////////////////////////////
size_t MeshPacker::maxProgress() const
{
	size_t progress = 0;
	for (unsigned i = 0; i < _nodes.numNodes(); i++)
	{
		Node* node = _nodes.getNode(i);
		QVector3D max = _nodes.getGeometry() - node->getMesh()->getGeometry();
		progress += max.x() * max.y();
	}
	return progress;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MeshPacker::run()
{
	Image base(_nodes.getGeometry().x(), _nodes.getGeometry().y(), /* clear color = */ 0.);
	std::atomic<size_t> progress_atom;
	progress_atom = 0;
	for (size_t i = 0; i < _nodes.numNodes(); i++)
	{
		Node* node = _nodes.getNode(i);
		emit report(QString("processing Mesh \"%1\"").arg(node->getMesh()->getName()));
		QVector3D max = _nodes.getGeometry() - node->getMesh()->getGeometry();

		Image::ColorType best_z = base.computeMinZ(0, 0, *(node->getBottom()));
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

				Image::ColorType z = base.computeMinZ(x, y, *(node->getBottom()));
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
		base.insertAt(best_x, best_y, best_z, *(node->getTop()));
		QVector3D best = QVector3D(best_x, best_y, best_z) - node->getMesh()->getMin();
		node->setPos(best);
	}

	emit processingDone();
}
