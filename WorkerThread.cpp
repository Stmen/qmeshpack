#include <QAtomicInt>
#include <QDateTime>
#include <QtConcurrentMap>
#include <QCoreApplication>
#include <cassert>
#include <functional>
#include <QStringList>
#include <QSettings>
#include <stdexcept>
#include <cstring>
#include <vector>
#include "WorkerThread.h"
#include "config.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////
WorkerThread::WorkerThread(QObject *parent, NodeModel &nodes) :
	QThread(parent), _task(ComputePositions), _nodes(nodes)
{
    connect(this, SIGNAL(nodePositionModified(uint)), &nodes, SLOT(nodePositionChanged(uint)), Qt::QueuedConnection);
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

			case MakeNormals:
				makeNormals();
				break;

			default:
				assert(0 && "bad task was selected, this should never happen");
				break;
		}
	}
	catch(const std::exception& ex)
	{
		report(tr("exception in worker thread: %1").arg(QString::fromUtf8(ex.what())), Console::Error);
	}
	catch(...)
	{
		report(tr("unknown exception in worker thread!"), Console::Error);
	}

	_lastProcessingMSecs = QDateTime::currentDateTime().toMSecsSinceEpoch() - time;
	emit processingDone();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void WorkerThread::makeNormals()
{
    emit reportProgressMax(_nodes.numNodes());

    QAtomicInt progress_atom(0);

#ifdef USE_OPENMP
	size_t num_nodes = _nodes.numNodes();
	#pragma omp parallel for
	for (unsigned i = 0; i < num_nodes; i++)
	{
		Mesh* mesh = _nodes.getNode(i)->getMesh();
		if (not mesh->hasNormals())
		{
			emit report(QString("processing mesh \"%1\"").arg(mesh->getName()), Console::Info);
			mesh->buildNormals();
		}
		emit reportProgress(progress_atom.fetchAndAddRelaxed(1));
	}
#else
    std::function<void (Node* node)> mapBuildNormals =
    [this, &progress_atom](Node* node)
    {
        Mesh* mesh = node->getMesh();
        if (not mesh->hasNormals())
        {
            emit report(QString("processing mesh \"%1\"").arg(mesh->getName()), 0);
            mesh->buildNormals();
        }
        emit reportProgress(progress_atom.fetchAndAddRelaxed(1));
    };

    QtConcurrent::blockingMap(_nodes.begin(), _nodes.end(), mapBuildNormals);
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void WorkerThread::saveNodeList()
{
	QString filename = _args.toString();
	if (filename.isEmpty())
	{
		report(tr("no save file given"), Console::Error);
		return;
	}

	emit reportProgressMax(_nodes.numNodes());

	const Node* node = _nodes.getNode(0);
	emit report(QString("processing mesh \"%1\"").arg(node->getMesh()->getName()), Console::Info);
    Mesh aggregate(*node->getMesh());

	for (unsigned i = 1; i < _nodes.numNodes(); i++)
	{
		emit reportProgress(i);
		node = _nodes.getNode(i);
		emit report(tr("processing mesh \"%1\"").arg(node->getMesh()->getName()), Console::Info);
        aggregate.add(*node->getMesh(), node->getPos());
		if (_shouldStop)
		{
			emit report(tr("saving aborted"), Console::Notify);
			break;
		}
	}
	aggregate.save(filename);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void WorkerThread::loadNodeList()
{
    QStringList qfilenames = _args.toStringList();
    if (qfilenames.isEmpty())
	{
		report(tr("no files given"), Console::Error);
		return;
	}

    QSettings settings(APP_VENDOR, APP_NAME);
    bool build_normals = settings.value("use_lighting", true).toBool();

    // QStringList is not thread safe even for access
    std::vector<QString> filenames;
    for (long i = 0; i < qfilenames.size(); i++)
        filenames.push_back(qfilenames[i]);

	emit reportProgressMax(filenames.size());
	QAtomicInt progress_atom(0);

	// mapper
#ifdef SINGLE_THREADED_LOADING
	for (size_t i = 0; i < filenames.size(); i++)
	{
		QStringList slist =  filenames[i].split(';');
		Node* node = new Node(slist[0].toUtf8().constData(), _nodes.getDefaultDilationValue());
        if (build_normals)
            node->getMesh()->buildNormals();
		if (slist.size() == 4)
			node->setPos(QVector3D(slist[1].toDouble(), slist[2].toDouble(), slist[3].toDouble()));

		emit reportProgress(progress_atom.fetchAndAddRelaxed(1));
		_nodes.addNode(node);
	}
#elif defined USE_OPENMP
	size_t fsize = filenames.size();

	bool abort = false;

	#pragma omp parallel for
	for (size_t i = 0; i < fsize; i++)
	{
		#pragma omp flush (abort)
		if (not abort)
		{
			if (_shouldStop)
			{
				emit report(tr("aborting!"), Console::Info);
				abort = true;
			#pragma omp flush (abort)
			}

			QStringList slist =  filenames[i].split(';');
            assert(not slist.isEmpty());

			Node* node = new Node(slist[0].toUtf8().constData(), _nodes.getDefaultDilationValue());
            if (build_normals)
                node->getMesh()->buildNormals();

			if (slist.size() == 4)
				node->setPos(QVector3D(slist[1].toDouble(), slist[2].toDouble(), slist[3].toDouble()));

			emit reportProgress(progress_atom.fetchAndAddRelaxed(1));
			emit report(tr("loaded %1").arg(node->getMesh()->getName()), Console::Info);
#pragma omp critical
			_nodes.addNode(node);
		}
	}
#else
	std::function<Node* (const QString& str)> mapCreateNode =
		[this, &progress_atom, build_normals](const QString& str)
		{
			QStringList slist =  str.split(';');
			Node* node = new Node(slist[0].toUtf8().constData(), _nodes.getDefaultDilationValue());
            if (build_normals)
                node->getMesh()->buildNormals();
			if (slist.size() == 4)
				node->setPos(QVector3D(slist[1].toDouble(), slist[2].toDouble(), slist[3].toDouble()));

			emit reportProgress(progress_atom.fetchAndAddRelaxed(1));
			return node;

		};

	// reducer
	std::function<void (int, Node*)> reduceAddNode =
			[this](int, Node* node) { _nodes.addNode(node); };

	_future = QtConcurrent::mappedReduced<int>(filenames, mapCreateNode, reduceAddNode, QtConcurrent::UnorderedReduce);
	_future.waitForFinished();
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
bool WorkerThread::nodeFits(const Node* node) const
{
	QVector3D geometry = _nodes.getGeometry();
	return	(node->getTop()->getWidth() <= geometry.x()) and
			(node->getTop()->getHeight() <= geometry.y()) and
			((node->getTop()->maxColor() - node->getBottom()->minColor()) <= geometry.z());

}

#ifdef USE_OPENMP
#include <omp.h>
/////////////////////////////////////////////////////////////////////////////////////////////////////
void WorkerThread::shouldStop()
{
	_shouldStop = true;
	#pragma omp flush (_shouldStop)
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void WorkerThread::computePositions()
{
    omp_set_num_threads(QThread::idealThreadCount());

	{	
        #ifdef REPORT_FINE_PROGRESS
        size_t max_progress = 0;
		for (unsigned i = 0; i < _nodes.numNodes(); i++)
		{
			Node* node = _nodes.getNode(i);
			node->setPos(QVector3D(0., 0., 0.));
            QVector3D max = _nodes.getGeometry() - node->getMesh()->getGeometry();
			max_progress += max.x() * max.y();
		}
        emit reportProgressMax(max_progress);
        #else
        emit reportProgressMax(_nodes.numNodes());
        #endif
	}

	Image base(_nodes.getGeometry().x(), _nodes.getGeometry().y());
	base.setAllPixelsTo(0.);
	double max_height = -INFINITY;

	QAtomicInt progress_atom(0);	

	for (size_t i = 0; i < _nodes.numNodes() and not _shouldStop; i++)
	{
		Node* node = _nodes.getNode(i);
		emit report(tr("processing Mesh ") + node->getMesh()->getName(), Console::Info);

		Image::ColorType best_z = INFINITY;
		unsigned best_x = 0;
		unsigned best_y = 0;
		float threshold = -INFINITY;

		if (not nodeFits(node))
		{
			emit report(QString("mesh ") + node->getMesh()->getName() + tr(" does not fit at all."), Console::Error);
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
						emit report(tr("aborting!"), Console::Info);
						abort = true;
						#pragma omp flush (abort)
					}

					const Image* bottom = node->getBottom();
					Image::offset_info info = base.findMinZDistanceAt(x, y, node->getBottom(), threshold);
					Image::ColorType z = bottom->at(info.x, info.y) - info.offset;

                    #ifdef REPORT_FINE_PROGRESS
                    emit reportProgress(progress_atom.fetchAndAddRelaxed(1));
                    #endif

					if (not info.early_rejection)
					{
						#pragma omp critical
						{
							if ((z < best_z) or (z == best_z and y < best_y) or (z == best_z and y == best_y and x < best_x))
							{
								best_z = z;
								best_y = y;
								best_x = x;
								threshold = info.offset;
								double h = best_z - node->getMesh()->getMin().z()
												   + node->getMesh()->getMax().z()
												   + node->getDilationValue();
								if (h > max_height)
									max_height = h;
							}
						}
					}
				}
			}
		}

#ifndef REPORT_FINE_PROGRESS
        emit reportProgress(progress_atom.fetchAndAddRelaxed(1));
#endif
		assert(best_x + node->getTop()->getWidth() <= base.getWidth());
		assert(best_y + node->getTop()->getHeight() <= base.getHeight());


        QVector3D newPos = QVector3D(best_x, best_y, best_z) - node->getMesh()->getMin() +
				QVector3D(node->getDilationValue(), node->getDilationValue(), node->getDilationValue());

		if ((newPos + node->getMesh()->getMax()).z()> _nodes.getGeometry().z())
		{
			emit report(tr("mesh ") + node->getMesh()->getName() + tr(" does not fit."), Console::Error);
			break;
		}
		// */

		base.insertAt(best_x, best_y, best_z, *(node->getTop()));
		node->setPos(newPos);
        #pragma omp taskyield
        emit nodePositionModified(i);
	}
	#pragma omp flush
	emit report(tr("max height is %1").arg(max_height), Console::Info);
}

#else
/////////////////////////////////////////////////////////////////////////////////////////////////////
void WorkerThread::shouldStop()
{
	_shouldStop = true;
	_future.cancel();   
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void WorkerThread::computePositions()
{
	{		
        #ifdef REPORT_FINE_PROGRESS
        size_t max_progress = 0;
        for (size_t i = 0; i < _nodes.numNodes(); i++)
		{
			Node* node = _nodes.getNode(i);
			node->setPos(QVector3D(0., 0., 0.));
			QVector3D max = _nodes.getGeometry() - node->getMesh()->getGeometry();
			max_progress += max.x() * max.y();            
		}
        emit reportProgressMax(max_progress);
        #else
        emit reportProgressMax(_nodes.numNodes());
        #endif
	}

	Image base(_nodes.getGeometry().x(), _nodes.getGeometry().y());
	base.setAllPixelsTo(0.);

	QAtomicInt progress_atom(0);
	for (size_t i = 0; i < _nodes.numNodes(); i++)
	{
		Node* node = _nodes.getNode(i);
        //emit report(QString("processing Mesh \"%1\"").arg(node->getMesh()->getName()), 0);

		Image::ColorType best_z = INFINITY;
		unsigned best_x = 0;
		unsigned best_y = 0;
		float threshold = -INFINITY;

		if (not nodeFits(node))
		{
			emit report(QString("mesh \"%1\" does not fit at all.").arg(node->getMesh()->getName()), 2);
			break;
		}

		unsigned max_y = _nodes.getGeometry().y() - node->getTop()->getHeight();
		unsigned max_x = _nodes.getGeometry().x() - node->getTop()->getWidth();

		struct xyz_t { quint32 x, y; Image::ColorType z; Image::ColorType offset; bool rejected; };

		std::function<xyz_t (quint64)> mapComputeZ =
			[this, &base, &progress_atom, &node, &threshold](quint64 coord)
			{
				quint32 x = (quint32)(coord & 0xFFFFFFFFU);
				quint32 y = (quint32)((coord >> 32) & 0xFFFFFFFFU);
				//qDebug() << QString("%1: %2 %3").arg(coord, 0, 16).arg(x, 0, 16).arg(y, 0, 16);
				const Image* bottom = node->getBottom();
				Image::offset_info info = base.findMinZDistanceAt(x, y, node->getBottom(), threshold);
				Image::ColorType z = bottom->at(info.x, info.y) - info.offset;

				//((*istart) & 0xFFFFFFFFU) << " y: " << (((*istart) >> 32) & 0xFFFFFFFFU)
				xyz_t out = { x, y, z, info.offset, info.early_rejection };

                #ifdef REPORT_FINE_PROGRESS
                emit reportProgress(progress_atom.fetchAndAddRelaxed(1));
                #endif
				return out;
			};

		std::function<void (int, xyz_t)> reduceBest = [&best_x, &best_y, &best_z, &threshold](int a, xyz_t xyz)
		{
			if (not xyz.rejected)
			{
				(void)a;
				if (xyz.z < best_z)
				{
					best_z = xyz.z;
					best_y = xyz.y;
					best_x = xyz.x;
					threshold = xyz.offset;
				}
				else if (xyz.z == best_z)
				{
					if (xyz.y < best_y)
					{
						best_y = xyz.y;
						best_x = xyz.x;
						threshold = xyz.offset;
					}
					else if (xyz.y == best_y)
					{
						if (xyz.x < best_x)
						{
							best_x = xyz.x;
							threshold = xyz.offset;
						}
					}
				}
			}
		};
#ifndef REPORT_FINE_PROGRESS
        emit reportProgress(progress_atom.fetchAndAddRelaxed(1));
#endif

		DoubleRangeIterator istart(0, 0, 0, max_x, 0, max_y);
		DoubleRangeIterator iend = istart.end();
		_future = QtConcurrent::mappedReduced<int>(istart, iend, mapComputeZ, reduceBest, QtConcurrent::UnorderedReduce);
		_future.waitForFinished();
		if (_shouldStop)
			break;

		//QCoreApplication::processEvents();

		assert(best_x + node->getTop()->getWidth() <= base.getWidth());
		assert(best_y + node->getTop()->getHeight() <= base.getHeight());


		QVector3D newPos = QVector3D(best_x, best_y, best_z) - node->getMesh()->getMin() +
				QVector3D(node->getDilationValue(), node->getDilationValue(), node->getDilationValue());

		if ((newPos + node->getMesh()->getMax()).z() > _nodes.getGeometry().z())
		{
			emit report(QString("mesh \"%1\" does not fit.").arg(node->getMesh()->getName()), 2);
			break;
		}

		base.insertAt(best_x, best_y, best_z, *(node->getTop()));

		node->setPos(newPos);
        emit nodePositionModified(i);
	}
}
#endif

