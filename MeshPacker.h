#ifndef MESHPACKER_H
#define MESHPACKER_H
#include <QThread>
#include "image.h"
#include "MeshFilesModel.h"

class MeshPacker : public QThread
{
	Q_OBJECT

public:
	explicit MeshPacker(NodeModel& nodes, QObject *parent = 0);
	void		setNodeList(NodeModel& nodes);
	size_t		maxProgress() const;

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

	NodeModel&			_nodes;
	bool				_shouldStop;
};

#endif // MESHPACKER_H
