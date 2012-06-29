#include "MeshPacker.h"
#include <QAtomicInt>
#include <QDateTime>
#include <QtConcurrentMap>
#include <cassert>
#include <functional>
#include <QStringList>
#include <stdexcept>
#include <omp.h>
#include "config.h"

template <class T, T min_x, T max_x>
union RangeIterator2
{
    T y;
    T x;

    std::pair<T, T> operator*() const
    {
        return std::make_pair(x, y);
    }

    bool operator==(const RangeIterator2& other) const
    {
        return (x == other.x) and (y == other.y);
    }

    void operator++()
    {
        x++;
        if (x == max_x)
            x == min_x;
    }

    /// Advances the iterator by n items
    void operator+=(size_t n)
    {

    }


    // --i	Moves the iterator back by one item
    void operator--()
    {
        if (x == min_x)
        {
            x = max_x;
            y--;
        }
    }

    //i -= n	Moves the iterator back by n items
    void operator-=(size_t n)
    {

    }

    // i - j	Returns the number of items between iterators i and j
    ptrdiff_t operator-(const RangeIterator2& other)
    {
        return (y - other.y) * (max_x - min_x) + (other.x - x);
    }

};

/////////////////////////////////////////////////////////////////////////////////////////////////////
WorkerThread::WorkerThread(QObject *parent, NodeModel &nodes) :
	QThread(parent), _task(ComputePositions), _nodes(nodes)
{
    omp_set_num_threads(QThread::idealThreadCount());
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void WorkerThread::saveNodeList()
{
	QString filename = _args.toString();
	if (filename.isEmpty())
	{
		report(tr("no save file given"), 2);
		return;
	}

	emit reportProgressMax(_nodes.numNodes());

	const Node* node = _nodes.getNode(0);
	//emit report(QString("processing Mesh \"%1\"").arg(node->getMesh()->getName()), 0);
	Mesh aggregate(*node->getMesh());

	for (unsigned i = 1; i < _nodes.numNodes(); i++)
	{
		emit reportProgress(i);
		node = _nodes.getNode(i);
		//emit report(QString("processing Mesh \"%1\"").arg(node->getMesh()->getName()), 0);
		aggregate.add(*node->getMesh(), node->getPos());
	}
	aggregate.save(filename);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void WorkerThread::loadNodeList()
{
	QStringList filenames = _args.toStringList();
	if (filenames.isEmpty())
	{
		report(tr("no files given"), 2);
		return;
	}

	emit reportProgressMax(filenames.size());
	QAtomicInt progress_atom(0);

	// mapper
#ifdef SINGLE_THREADED_LOADING
	for (long i = 0; i < filenames.size(); i++)
	{
		QStringList slist =  filenames[i].split(';');
		Node* node = new Node(slist[0].toUtf8().constData(), _nodes.getDefaultDilationValue());
		if (slist.size() == 4)
			node->setPos(QVector3D(slist[1].toDouble(), slist[2].toDouble(), slist[3].toDouble()));

		emit reportProgress(progress_atom.fetchAndAddRelaxed(1));
		_nodes.addNode(node);
	}
#else
	std::function<Node* (const QString& str)> nodeCreate =
		[this, &progress_atom](const QString& str)
		{
			QStringList slist =  str.split(';');
			Node* node = new Node(slist[0].toUtf8().constData(), _nodes.getDefaultDilationValue());
			if (slist.size() == 4)
				node->setPos(QVector3D(slist[1].toDouble(), slist[2].toDouble(), slist[3].toDouble()));

			emit reportProgress(progress_atom.fetchAndAddRelaxed(1));
			return node;

		};

	// reducer
	std::function<void (int, Node*)> nodeAdd =
			[this](int a, Node* node) { (void)a; _nodes.addNode(node); };


	QtConcurrent::blockingMappedReduced<int>(filenames, nodeCreate, nodeAdd, QtConcurrent::UnorderedReduce);
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void WorkerThread::run()
{
	quint64 time = QDateTime::currentDateTime().toMSecsSinceEpoch();
	_shouldStop = false;
	try
	{
		switch (_task)
		{
			case ComputePositions:
				computePositions();
				break;

			case SaveMeshList:
				if (_args.isValid())
					saveNodeList();
				break;

			case LoadMeshList:
				if (_args.isValid())
					loadNodeList();
				break;

			default:
				assert(0 && "bad task was selected, this should never happen");
				break;
		}
	}
	catch(const std::exception& ex)
	{
		report(tr("exception in worker thred: ") + QString(ex.what()), 2);
	}
	catch(...)
	{
		report(tr("unknown exception in worker thred!"), 0);
	}

	_lastProcessingMSecs = QDateTime::currentDateTime().toMSecsSinceEpoch() - time;
	emit processingDone();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
bool WorkerThread::nodeFits(const Node* node) const
{
	QVector3D geometry = _nodes.getGeometry();
	return	(node->getTop()->getWidth() <= geometry.x()) and
			(node->getTop()->getHeight() <= geometry.y()) and
			((node->getTop()->maxColor() - node->getBottom()->minColor()) <= geometry.z());

}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void WorkerThread::computePositions()
{	
	{
		size_t max_progress = 0;
		for (unsigned i = 0; i < _nodes.numNodes(); i++)
		{
			Node* node = _nodes.getNode(i);
            //node->setPos(QVector3D(0., 0., 0.));
			QVector3D max = _nodes.getGeometry() - node->getMesh()->getGeometry();
			max_progress += max.x() * max.y();
		}

		emit reportProgressMax(max_progress);
	}

	Image base(_nodes.getGeometry().x(), _nodes.getGeometry().y());
	base.setAllPixelsTo(0.);

    QAtomicInt progress_atom(0);
	for (size_t i = 0; i < _nodes.numNodes(); i++)
	{
		Node* node = _nodes.getNode(i);
		emit report(QString("processing Mesh \"%1\"").arg(node->getMesh()->getName()), 0);

        Image::ColorType best_z = INFINITY;
		unsigned best_x = 0;
		unsigned best_y = 0;

		if (not nodeFits(node))
		{
			emit report(QString("mesh \"%1\" does not fit at all.").arg(node->getMesh()->getName()), 2);
			break;
		}

		unsigned max_y = _nodes.getGeometry().y() - node->getTop()->getHeight();
		unsigned max_x = _nodes.getGeometry().x() - node->getTop()->getWidth();

		bool abort = false;
        #pragma omp parallel for collapse(2)
		for (unsigned y = 0; y < max_y; y++)
		{
			for (unsigned x = 0; x < max_x; x++)
			{
				#pragma omp flush (abort)
				if (not abort)
				{
					//Update progress
					if (_shouldStop)
					{
						emit report(tr("aborting!"), 0);
						abort = true;
						#pragma omp flush (abort)
					}

					Image::ColorType z = base.computeMinZDistance(x, y, *(node->getBottom()));
                    emit reportProgress(progress_atom.fetchAndAddRelaxed(1));

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
		}

        assert(best_x + node->getTop()->getWidth() <= base.getWidth());
        assert(best_y + node->getTop()->getHeight() <= base.getHeight());		


		QVector3D newPos = QVector3D(best_x, best_y, best_z) - node->getMesh()->getMin() +
				QVector3D(node->getDilationValue(), node->getDilationValue(), node->getDilationValue());

		if ((newPos + node->getMesh()->getGeometry()).z() > _nodes.getGeometry().z())
		{
			emit report(QString("mesh \"%1\" does not fit.").arg(node->getMesh()->getName()), 2);
			break;
		}

        base.insertAt(best_x, best_y, best_z, *(node->getTop()));

		node->setPos(newPos);
        _nodes.nodePositionChanged(i);
	}
    #pragma omp flush
}
