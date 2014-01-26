#pragma once
#include <QThread>
#include <QVariant>
#include <QFuture>
//#include <QtConcurrentMap>
#include <Console.h>
#include "NodeModel.h"

class WorkerThread : public QThread
{
	Q_OBJECT

public:

	enum Task
	{
		ComputePositions,
		SaveMeshList,
        LoadMeshList,
        MakeNormals
	} _task;

    explicit        WorkerThread(QObject *parent, NodeModel &nodes);
    inline void     setTask(Task task) { _task = task; }
    inline void     setArgument(QVariant arg) { _args = arg; }
    inline quint64	getLastProcessingMSecs() const { return _lastProcessingMSecs; }

protected:

	void run();

signals:

	void reportProgressMax(int);
	void reportProgress(int progress);
	void processingDone();
	void report(QString what, Console::InfoLevel level);
    void nodePositionModified(unsigned idx);

public slots:

	void shouldStop();

private:

    void    makeNormals();
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
