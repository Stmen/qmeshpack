#ifndef MESHPACKER_H
#define MESHPACKER_H
#include <QThread>
#include "image.h"
#include "MeshFilesModel.h"

class WorkerThread : public QThread
{
	Q_OBJECT

public:

	enum Task
	{
		ComputePositions,
		SaveMeshList
	} _task;

	explicit WorkerThread(NodeModel& nodes, QObject *parent = 0);
	void	setNodeList(NodeModel& nodes);
	size_t	maxProgress() const;
	void	setTask(Task task) { _task = task; }
	void	setArgument(QString arg) { _arg = arg; }
	quint64	getLastProcessingMSecs() const { return _lastProcessingMSecs; }
protected:
	void run();

signals:

	void reportProgress(int progress);
	void processingDone();
	void report(QString what, unsigned level);	

public slots:
	void shouldStop()
	{
		_shouldStop = true;
		#pragma omp flush (_shouldStop)
	}

private:

	void computePositions();
	void saveNodeList();

	NodeModel&			_nodes;
	bool				_shouldStop;
	QString				_arg;
	quint64				_lastProcessingMSecs;
};

#endif // MESHPACKER_H
