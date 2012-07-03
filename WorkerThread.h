#pragma once
#include <QThread>
#include <QVariant>
#include <QtConcurrentMap>
#include "NodeModel.h"

class WorkerThread : public QThread
{
	Q_OBJECT

public:

	enum Task
	{
		ComputePositions,
		SaveMeshList,
		LoadMeshList
	} _task;

	explicit WorkerThread(QObject *parent, NodeModel &nodes);
	void	setNodeList(NodeModel& nodes);
	void	setTask(Task task) { _task = task; }
	void	setArgument(QVariant arg) { _args = arg; }
	quint64	getLastProcessingMSecs() const { return _lastProcessingMSecs; }

protected:
	void run();

signals:

	void reportProgressMax(int);
	void reportProgress(int progress);
	void processingDone();
	void report(QString what, unsigned level);

public slots:

	void shouldStop();

private:

	void	computePositions();
	void	saveNodeList();
	void	loadNodeList();
	bool	nodeFits(const Node* node) const;

	QFuture<void>		_future;
	NodeModel&			_nodes;
	volatile bool		_shouldStop;
	QVariant			_args;
	quint64				_lastProcessingMSecs;
};
