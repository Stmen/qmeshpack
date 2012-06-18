#include "MeshPacker.h"
#include <cassert>
#include <omp.h>
#include <QAtomicInt>


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
    QAtomicInt progress_atom(0);
	for (size_t i = 0; i < _nodes.numNodes(); i++)
	{
		Node* node = _nodes.getNode(i);
		emit report(QString("processing Mesh \"%1\"").arg(node->getMesh()->getName()));

        Image::ColorType best_z = INFINITY;
		unsigned best_x = 0;
		unsigned best_y = 0;

		unsigned max_y = _nodes.getGeometry().y() - node->getTop()->getHeight();
		unsigned max_x = _nodes.getGeometry().x() - node->getTop()->getWidth();

        #pragma omp parallel for collapse(2)
		for (unsigned y = 0; y < max_y; y++)
		{
			for (unsigned x = 0; x < max_x; x++)
			{
                emit reportProgress(progress_atom.fetchAndAddOrdered(1));
				Image::ColorType z = base.computeMinZ(x, y, *(node->getBottom()));
				#pragma omp critical
				{
					if (z < best_z)
					{
						best_z = z;
                        best_y = y;
						best_x = x;						
					}
                    else if (z == best_z)
                    {
                        if (y < best_y)
                        {
                            best_y = y;
                            best_x = x;
                        }
                        else if (y == best_y)
                        {
                            if (x < best_x)
                            {
                                best_x = x;
                            }
                        }
                    }
				}
			}
		}

        assert(best_x + node->getTop()->getWidth() <= base.getWidth());
        assert(best_y + node->getTop()->getHeight() <= base.getHeight());

        base.insertAt(best_x, best_y, best_z, *(node->getTop()));

		QVector3D newPos = QVector3D(best_x, best_y, best_z) - node->getMesh()->getMin() +
				QVector3D(node->getDilationValue(), node->getDilationValue(), node->getDilationValue());
		node->setPos(newPos);
		_nodes.nodeChanged(i);
	}

	emit processingDone();
    #pragma omp flush
}
