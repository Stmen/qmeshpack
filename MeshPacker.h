#ifndef MESHPACKER_H
#define MESHPACKER_H
#include <QThread>
#include "image.h"
#include "MeshFilesModel.h"

class MeshPacker : public QThread
{
	Q_OBJECT

public:
	explicit MeshPacker(MeshFilesModel& nodes, QObject *parent = 0);
	void		setNodeList(MeshFilesModel& nodes);
	size_t		maxProgress() const;

protected:
	void run();

signals:

	void reportProgress(int progress);
	void processingDone();
	void report(QString what);

public slots:

private:

	MeshFilesModel&			_nodes;
};

#endif // MESHPACKER_H
