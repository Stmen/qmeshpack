#include "mainwindow.h"
#include <QProgressBar>
#include <QPainter>
#include <QSettings>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QDateTime>
#include <QScrollBar>
#include <QDebug>
#include <QDesktopWidget>
#include <QImage>
#include <QtConcurrentRun>
#include <QtConcurrentMap>
#include <QWidgetAction>
#include <functional> // I love lamdas.
#include <cmath>
#include <cstdlib>
#include <cassert>
#include "image.h"
#include "GLView.h"
#include "Exception.h"
#include "config.h"

/// theese are different views in _stack. I used the QStack to avoid unnatural geometry changes while switching different views
#define VIEW_WELCOME 0
#define VIEW_RESULTS 1
#define VIEW_MODEL 2

///////////////////////////////////////////////////////////////////////////////////////////////////
///
///	This is mostly a small test routine to see the rendering of the triangles.
///
///////////////////////////////////////////////////////////////////////////////////////////////////
void drawIntro(Image* img, QVector3D offset, double from, double r1, double r2)
{
    for (double i = from; i < from + 3.1415926f * 2; i += 0.8)
	{
		double x1 = cos(i), y1 = sin(i);
		double x2 = cos(i + 0.9), y2 = sin(i + 0.8);

		/*
		QVector3D a = QVector3D(x1 * r1,			y1 * r1, rand() & 0xFF) + offset;
		QVector3D b = QVector3D(x2 * r1,			y2 * r1, rand() & 0xFF) + offset;
		QVector3D c = QVector3D((x1 + x2) / 2 * r2, (y1 + y2) / 2 * r2, rand() & 0xFF) + offset;
		*/
		QVector3D a = offset + QVector3D(0., 0., rand() & 0xFF);
		QVector3D b = QVector3D(x2 * r1,			y2 * r1, rand() & 0xFF) + offset;
		QVector3D c = QVector3D((x1 + x2) / 2 * r2, (y1 + y2) / 2 * r2, rand() & 0xFF) + offset;
		img->triangle(a, b, c, Image::maxValue);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
MainWindow::MainWindow() :
	QMainWindow(),
	_stack(new QStackedWidget(this))
{
	QSettings settings("Konstantin", APP_NAME);
	restoreGeometry(settings.value("geometry").toByteArray());
	restoreState(settings.value("windowState").toByteArray());
	_conversionFactor = qvariant_cast<float>(settings.value("conversionFactor", 1.));
	_defaultDilationValue = qvariant_cast<unsigned>(settings.value("dilation", 00));

	QGLFormat fmt;
	fmt.setAlpha(true);
	fmt.setStereo(false);
	fmt.setSampleBuffers(true);
	QGLFormat::setDefaultFormat(fmt);

	setCentralWidget(_stack);
	QVector3D box_geom = qvariant_cast<QVector3D>(settings.value("box_geometry", QVector3D(1000., 1000., 1000.)));
	_modelMeshFiles = new NodeModel(box_geom, this);

	createMeshList();

	// creating Mesh Packer	
	_threadWorker = new WorkerThread(*_modelMeshFiles, this);
	connect(_threadWorker, SIGNAL(processingDone()), this, SLOT(processNodesDone()));
	connect(_threadWorker, SIGNAL(report(QString, unsigned)), this, SLOT(consolePrint(QString, unsigned)));

	// create console
	QDockWidget* dockWidget = new QDockWidget(tr("Console"), this);
	_console = new QTextEdit(this);
	_console->setCurrentCharFormat(QTextCharFormat());
	dockWidget->setWidget(_console);
	addDockWidget(Qt::BottomDockWidgetArea, dockWidget);

	createStack();	
	createActions();
	createMenusAndToolbars(); // menus depend on existing actions and some widgets like _progress*

	bool doScaleImages = qvariant_cast<bool>(settings.value("scaleImages"));
	_viewModel->setScaleImages(doScaleImages);
	_actDoScaleImages->setChecked(doScaleImages);

	connect(_modelMeshFiles, SIGNAL(geometryChanged()), this, SLOT(updateWindowTitle()));
	connect(_modelMeshFiles, SIGNAL(numNodesChanged()), this, SLOT(updateWindowTitle()));
	updateWindowTitle();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
MainWindow::~MainWindow()
{
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::saveScreenshot()
{
	_viewModel->getGLView()->flushGL();
	QPixmap screenshot = QPixmap::grabWindow(QApplication::desktop()->winId()).copy(geometry());

	QString filename = QFileDialog::getSaveFileName(this,
		tr("Save main window screenshot"), "",
		tr("PNG (*.png);; JPG (*.jpg);; All Files (*)"));

	if (filename.isEmpty())
		return;

	// what if the users did not specify a suffix...?
	QFileInfo fileInfo(filename);
	if (fileInfo.suffix().isEmpty())
		filename += ".png";

	screenshot.save(filename);

	/* This way doesn't work for QGLView
	QImage img(size(), QImage::Format_RGB32);
	QPainter painter(&img);
	render(&painter);
	img.save(filename);
	// */
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::closeEvent(QCloseEvent *event)
{
	QSettings settings(APP_VENDOR, APP_NAME);
	settings.setValue("geometry", saveGeometry());
	settings.setValue("windowState", saveState());
	settings.setValue("box_geometry", _modelMeshFiles->getGeometry());
	settings.setValue("conversionFactor", _conversionFactor);
	settings.setValue("dilation", _defaultDilationValue);
	settings.setValue("scaleImages", _viewModel->areImagesScaled());
	QMainWindow::closeEvent(event);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::createMeshList()
{
	_viewMeshFiles = new QTreeView(this);
	_viewMeshFiles->setModel(_modelMeshFiles);
	_viewMeshFiles->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	//_viewMeshFiles->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Ignored);
	_viewMeshFiles->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(_viewMeshFiles, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(menuContextNode(const QPoint &)));
	connect(_viewMeshFiles, SIGNAL(clicked(const QModelIndex &)), this, SLOT(mainNodeSelected(const QModelIndex &)));
	QItemSelectionModel* selectionModel = _viewMeshFiles->selectionModel();
	connect(selectionModel, SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)), this, SLOT(mainNodeSelected(const QModelIndex &)));
	QString mname = tr("Mesh list");
	QDockWidget* dockWidget = new QDockWidget(mname, this);
	dockWidget->setObjectName(mname);
	//dockWidget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Ignored);
	dockWidget->setWidget(_viewMeshFiles);
	addDockWidget(Qt::LeftDockWidgetArea, dockWidget);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::createActions()
{
	QStyle* style = QApplication::style();

	_actStop = new QAction(QIcon(":/trolltech/styles/commonstyle/images/stop-32.png"), tr("&Stop"), this);
	_actStop->setEnabled(false);
	connect(_actStop, SIGNAL(triggered()), _threadWorker, SLOT(shouldStop()));

	_actExit = new QAction(QIcon(), tr("E&xit"), this);
	_actExit->setShortcuts(QKeySequence::Close);
	_actExit->setStatusTip(tr("Exits this Application"));
	connect(_actExit, SIGNAL(triggered()), qApp, SLOT(quit()));

	_actAddFile = new QAction(style->standardIcon(QStyle::SP_DialogOpenButton), tr("&Add mesh"), this);
	_actAddFile->setShortcuts(QKeySequence::Open);
	_actAddFile->setStatusTip(tr("Adds a new Mesh"));
	connect(_actAddFile, SIGNAL(triggered()), this, SLOT(dialogAddMesh()));

	_actProcess = new QAction(QIcon(":/trolltech/styles/commonstyle/images/media-play-32.png" ), tr("&Process"), this);
	_actProcess->setShortcuts(QKeySequence::Refresh);
	_actProcess->setStatusTip(tr("Starts combining the Meshes."));
	connect(_actProcess, SIGNAL(triggered()), this, SLOT(processNodes()));

	_actSetBoxGeometry = new QAction(QIcon(), tr("Set &geometry"), this);
	_actSetBoxGeometry->setStatusTip(tr("Sets the bounding box geometry for the target space."));
	connect(_actSetBoxGeometry, SIGNAL(triggered()), this, SLOT(dialogSetBoxGeometry()));

	_actSetConversionFactor = new QAction(QIcon(), tr("Set default &conversion factor"), this);
	_actSetConversionFactor->setStatusTip(tr("Sets the default conversion factor"));
	connect(_actSetConversionFactor, SIGNAL(triggered()), this, SLOT(dialogSetConversionFactor()));

	_actSetDefaultDilationValue = new QAction(QIcon(), tr("Set default &dilation value"), this);
	_actSetDefaultDilationValue->setStatusTip(tr("Sets the default dilation value."));
	connect(_actSetDefaultDilationValue, SIGNAL(triggered()), this, SLOT(dialogSetDefaultDilation()));
	_actShowResults = new QAction(QIcon(":/trolltech/styles/commonstyle/images/viewdetailed-32.png"), tr("Show &results"), this);
	_actShowResults->setStatusTip(tr("Shows results in the main window"));
	connect(_actShowResults, SIGNAL(triggered()), this, SLOT(mainShowResults()));

	_actSaveScreenshot = new QAction(QIcon(":images/images/camera.png"), tr("Save &screenshot"), this);
	_actSaveScreenshot->setStatusTip(tr("Saves a screenshot of a main window"));
	connect(_actSaveScreenshot, SIGNAL(triggered()), this, SLOT(saveScreenshot()));

	_actSaveResults = new QAction(style->standardIcon(QStyle::SP_DialogSaveButton), tr("Save &results"), this);
	_actSaveResults->setStatusTip(tr("Saves a result positions"));
	connect(_actSaveResults, SIGNAL(triggered()), this, SLOT(dialogSaveResults()));

	// creating Mesh specific actions
	_actMeshRemove = new QAction(style->standardIcon(QStyle::SP_DialogCloseButton), tr("&Remove current mesh"), this);
	_actMeshRemove->setShortcuts(QKeySequence::Delete);
	_actMeshRemove->setStatusTip(tr("Removes this Mesh"));
	connect(_actMeshRemove, SIGNAL(triggered()), this, SLOT(removeCurrentNode()));

	_actMeshTranslate = new QAction(QIcon(), tr("&Translate"), this);
	_actMeshTranslate->setStatusTip(tr("Translate this mesh"));
	connect(_actMeshTranslate, SIGNAL(triggered()), this, SLOT(transformCurrentMesh()));

	_actMeshScale = new QAction(QIcon(), tr("&Scale"), this);
	_actMeshScale->setStatusTip(tr("Scale this mesh"));
	connect(_actMeshScale, SIGNAL(triggered()), this, SLOT(scaleCurrentMesh()));

	_actDoScaleImages = new QAction(QIcon(), tr("Scale &images"), this);
	_actDoScaleImages->setStatusTip(tr("Scale images"));
	_actDoScaleImages->setCheckable(true);
	connect(_actDoScaleImages, SIGNAL(toggled(bool)), _viewModel, SLOT(setScaleImages(bool)));

}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::createMenusAndToolbars()
{
	//-----------[ menus ]----------------
	QMenu* menu = new QMenu(tr("&File"));
	menu->insertAction(0, _actAddFile);
	menu->insertAction(0, _actSaveResults);
	menu->insertAction(0, _actProcess);
	menu->addSeparator();
	menu->insertAction(0, _actExit);
	menuBar()->addMenu(menu);

	menu = new QMenu(tr("&View"));
	menu->insertAction(0, _actShowResults);
	menuBar()->addMenu(menu);

	menu = new QMenu(tr("&Settings"));
	menu->insertAction(0, _actSetBoxGeometry);
	menu->insertAction(0, _actSetConversionFactor);
	menu->insertAction(0, _actSetDefaultDilationValue);
	menu->insertAction(0, _actDoScaleImages);
	menuBar()->addMenu(menu);

	menu = new QMenu(tr("&Help"));
	connect(menu->addAction(tr("About ") + "QT"), SIGNAL(triggered()), qApp, SLOT(aboutQt()));
	connect(menu->addAction(tr("About ") + APP_NAME), SIGNAL(triggered()), this, SLOT(aboutThisApp()));
	menuBar()->addMenu(menu);

	//-----------[ toolbars ]----------------
	_toolMain = addToolBar(tr("Main toolbar"));
	_toolMain->insertAction(0, _actAddFile);
	_toolMain->insertAction(0, _actSaveResults);
	_toolMain->insertAction(0, _actProcess);
	_toolMain->insertAction(0, _actSaveScreenshot);
	_toolMain->insertAction(0, _actStop);
	_toolMain->insertAction(0, _actShowResults);
	_toolMain->addSeparator();

	//-----------[ progress bar inside the toolbar ]----------------
	QWidgetAction* action = new QWidgetAction(this);
	action->setDefaultWidget(_progressWidget);
	_toolMain->addAction(action);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::createStatusBar()
{	
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::createStack()
{
	QLabel* label = new QLabel();
	label->setBackgroundRole(QPalette::Base);
	//imageLabel->setSizePolicy(QSizePolicy:: Ignored, QSizePolicy::Ignored);
	//imageLabel->setScaledContents(false);
	label->setScaledContents(true);
	Image img(700, 700, 0);
	//testTriangle1(&img);
	//img.dilate(10, Image::maxValue);
	drawIntro(&img, QVector3D(img.getWidth() / 2, img.getHeight() / 2, 0), 0, 150, 300);
	//drawIntro(&img, QVector3D(img.getWidth() / 2, img.getHeight() / 2, 0), 0.2, 50, 300);
	//drawIntro(&img, QVector3D(img.getWidth() / 2, img.getHeight() / 2, 0), 0.4, 50, 300);

	QFont font("times", 20);
	QPixmap pixmap = QPixmap::fromImage(img.toQImage(),  Qt::ThresholdDither);
	QPainter painter(&pixmap);
	painter.setFont(font);
	painter.setPen(Qt::red);
	painter.drawText(0, 0, width(), height(), 0, APP_NAME);
	painter.end();

	label->setPixmap(pixmap);
	label->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	_stack->addWidget(label);

	GLView* boxView = new GLView(_modelMeshFiles);
	connect(_threadWorker, SIGNAL(processingDone()), boxView, SLOT(updateGL()));
	_stack->addWidget(boxView);
	_viewModel = new ModelView(this);
	_stack->addWidget(_viewModel);

	_progressWidget = new QProgressBar();
	_progressWidget->setEnabled(false);

	connect(_threadWorker, SIGNAL(reportProgress(int)), _progressWidget, SLOT(setValue(int)));
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::mainNodeSelected(const QModelIndex & index)
{
	if (index.isValid())
	{
		Node* node = (Node*)_modelMeshFiles->data(index, Qt::UserRole).value<void*>();
		_viewModel->setNode(node);
		_stack->setCurrentIndex(VIEW_MODEL);
	}
	else
	{
		_stack->setCurrentIndex(VIEW_RESULTS);
	}

}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::mainShowResults()
{
	_stack->setCurrentIndex(VIEW_RESULTS);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::menuContextNode(const QPoint & pos)
{
	QModelIndex idx = _viewMeshFiles->indexAt(pos);

	if(idx.isValid())
	{
		_currMeshIndex = idx;
		QMenu menu;
		menu.insertAction(0, _actMeshRemove);
		menu.insertAction(0, _actMeshScale);
		menu.insertAction(0, _actMeshTranslate);
		//menu.addSeparator();
		menu.exec(_viewMeshFiles->mapToGlobal(pos));
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::removeCurrentNode()
{
	if (_currMeshIndex.isValid())
	{
		_viewMeshFiles->clearSelection();
		_stack->setCurrentIndex(VIEW_RESULTS);
		unsigned idx = _currMeshIndex.row();
		consolePrint(tr("removing node \"%1\"").arg(_modelMeshFiles->getNode(idx)->getMesh()->getName()), 0);
		_modelMeshFiles->removeRows(idx, 1, QModelIndex());
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::transformCurrentMesh()
{
	QString msg = tr("transforming by");
	VectorInputDialog dialog(this, msg, "");
	int code = dialog.exec();
	if (code == 1) // accepted
	{
		Node* node = (Node*)_modelMeshFiles->data(_currMeshIndex, Qt::UserRole).value<void*>();
		msg += QString("(%1 %2 %3)").arg(dialog.getResult().x()).arg(dialog.getResult().y()).arg(dialog.getResult().z());
		consolePrint(msg);
		node->translateMesh(dialog.getResult());
		mainNodeSelected(_currMeshIndex);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::scaleCurrentMesh()
{
	QString msg = tr("Scaling by");
	VectorInputDialog dialog(this, msg, "", QVector3D(1., 1., 1.));

	int code = dialog.exec();
	if (code == 1) // accepted
	{
		Node* node = (Node*)_modelMeshFiles->data(_currMeshIndex, Qt::UserRole).value<void*>();
		msg += QString("(%1 %2 %3)").arg(dialog.getResult().x()).arg(dialog.getResult().y()).arg(dialog.getResult().z());
		consolePrint(msg);
		node->scaleMesh(dialog.getResult());
		mainNodeSelected(_currMeshIndex);
	}
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::dialogSetBoxGeometry()
{
	QVector3D conv(_conversionFactor, _conversionFactor, _conversionFactor);
	QVector3D invConv = QVector3D(1., 1., 1.) / _conversionFactor;

	VectorInputDialog dialog(this, tr("Box geometry in user defined units.\n"),
									  tr("Box geometry in user defined units.\n"
										 "Define your own units through the conversion factor\n"
										 "The resulting box in application units is then (x, y, z) * <conversion factor>."
										 ), _modelMeshFiles->getGeometry() * invConv);

	int code = dialog.exec();
	if (code == 1) // accepted
	{
		QVector3D userResult = floor(dialog.getResult());
		QVector3D result = floor(dialog.getResult() * conv);
		if (result.x() > 0 and result.y() > 0 and result.z() > 0)
		{
			QString userStr = QString("(%1 %2 %3)").arg(userResult.x()).arg(userResult.y()).arg(userResult.z());
			setWindowTitle(tr(APP_NAME) + tr("  box geometry: ") + userStr + tr(" user units."));
			QString resultStr = QString("(%1 %2 %3)").arg(result.x()).arg(result.y()).arg(result.z());
			consolePrint(tr("Setting box geometry to ") + resultStr + " application units.");
			_modelMeshFiles->setGeometry(result);
		}
		else
			consolePrint(tr("Bad input"), 2);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::dialogSetConversionFactor()
{
	bool ok;
	double f = QInputDialog::getDouble(this, tr("Set new conversion value"),
												 tr("new conversion value"),
												 _conversionFactor,
												 -1000000, 1000000, 4, &ok);
	if (ok and f != _conversionFactor)
	{
		_conversionFactor = f;
		consolePrint(tr("Setting new conversion factor to ") + QString::number(f));
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::dialogSetDefaultDilation()
{
	QString msg = tr("Setting default dilation value ");
	bool ok;
	double value = QInputDialog::getInteger(this, msg, tr("dilation value"), _defaultDilationValue,
												 0, 80, 1, &ok);
	if (ok)
	{
		_defaultDilationValue = value;
		consolePrint(msg + QString::number(value));
	}
}


struct NodeInfo
{
    QString filename;
    QVector3D position;
	unsigned dilation;
};

struct MeshInfo
{
	QString filename;
	QVector3D pos;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
static void txtToList(QString filename, QList<MeshInfo>& mesh_infos)
{
	QFile file(filename);
	if (not file.open(QIODevice::ReadOnly | QIODevice::Text))
		throw Exception("Unable to open file %s: %s", filename.toUtf8().constData(), file.errorString().toUtf8().constData());


	while (not file.atEnd() and file.isReadable())
	{
		QList<QByteArray> list = file.readLine().split(';');
		if (list.empty()) // ignore unparsable
			continue;

		QString  off_filename = list.at(0).trimmed();
		if (off_filename.isEmpty())
			continue;

		MeshInfo m = { off_filename, QVector3D(list.at(1).toDouble(), list.at(2).toDouble(), list.at(3).toDouble()) };
		mesh_infos.push_back(m);
	}
}



/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::dialogAddMesh()
{
	QFileDialog dialog(this,
		  /* caption = */ tr("Save resuslts as"),
		  /* directory = */ "",
		  "OFF meshes (*.off *.OFF);;"
		  "TXT mesh list(*.txt *.TXT)");

	dialog.setFileMode(QFileDialog::ExistingFiles);
	if (not dialog.exec())
		return;

	QStringList filenames = dialog.selectedFiles();
	if (filenames.isEmpty())
		return;

	std::function<void (int, Node*)> nodeAdd =
			[this](int a, Node* node) { (void)a; _modelMeshFiles->addNode(node); };

	try
	{
		if (dialog.selectedNameFilter().at(0) == 'O')
		{
			std::function<Node* (const QString& str)> nodeCreate =
					[this](const QString& str)
					{
						return new Node(str.toUtf8().constData(), _defaultDilationValue);
					};

			QtConcurrent::blockingMappedReduced<int>(filenames, nodeCreate, nodeAdd, QtConcurrent::UnorderedReduce);
		}
		else if (dialog.selectedNameFilter().at(0) == 'T')
		{
			QList<MeshInfo> mesh_infos;

			for (int i = 0; i < filenames.size(); i++)
				txtToList(filenames.at(i), mesh_infos);

			std::function<Node* (const MeshInfo& info)> nodeCreate =
					[this](const MeshInfo& info)
					{
						Node* node = new Node(info.filename.toUtf8().constData(), _defaultDilationValue);
						node->setPos(info.pos);
						return node;
					};

			QtConcurrent::blockingMappedReduced<int>(mesh_infos, nodeCreate, nodeAdd, QtConcurrent::UnorderedReduce);
		}
	}
	catch (const std::exception& ex)
	{
		QString msg  = QString("failed to open files: ") + QString(ex.what());
		consolePrint(msg, 2);
		QMessageBox::critical(this, tr("Open failed"), msg);
	}
	catch (...)
	{
		QString msg  = QString("failed to open files");
		consolePrint(QString(msg), 2);
		QMessageBox::critical(this, tr("Open failed"), msg);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::startWorker(WorkerThread::Task task, QString arg)
{
	_actAddFile->setEnabled(false);
	_actMeshRemove->setEnabled(false);
	_actMeshScale->setEnabled(false);
	_actMeshTranslate->setEnabled(false);
	_actProcess->setEnabled(false);
	_actSaveResults->setEnabled(false);
	_actSetBoxGeometry->setEnabled(false);
	_actStop->setEnabled(true);
	if (not arg.isEmpty())
		_threadWorker->setArgument(arg);
	_threadWorker->setTask(task);
	_progressWidget->setRange(0, _threadWorker->maxProgress());
	_progressWidget->setEnabled(true);
	_threadWorker->start();
}

#include <QDir>
/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::dialogSaveResults()
{
	if(_modelMeshFiles->numNodes() == 0)
	{
		consolePrint(tr("no nodes to save"), 2);
		return;
	}

	QString selectedFilter;
	QString filename = QFileDialog::getSaveFileName(this,
													tr("Choose a file to save to"),
													QDir::currentPath(),
													"TXT files (*.txt *.TXT);;"
													"STL Files(*.stl *.STL);;"
													"OFF Files(*.off *.OFF)",
													&selectedFilter);
	if (filename.isEmpty())
		return;

	selectedFilter.resize(3);
	selectedFilter = selectedFilter.toLower();

	// what if the users did not specify a suffix...?
	QFileInfo fileInfo(filename);
	if (fileInfo.suffix().isEmpty())	
		filename = filename + '.' + selectedFilter.toLower();

	if (selectedFilter.at(0) == 'o' or selectedFilter.at(0) == 's') // .off or .stl
	{		
		consolePrint(tr("saving results to an OFF mesh \"%1\"").arg(filename));
		startWorker(WorkerThread::SaveMeshList, filename);
	}
	else if (selectedFilter.at(0) == 't') // text of line with format filename,x,y,z
	{
		QFile file(filename);
		if (not file.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			QMessageBox::information(0, tr("Unable to open file"), file.errorString());
			return;
		}

		consolePrint(tr("saving results to a list \"%1\"").arg(filename));
		QTextStream out(&file);

		for (unsigned i = 0; i < _modelMeshFiles->numNodes(); i++)
		{
			Node* node = _modelMeshFiles->getNode(i);
			out << node->getMesh()->getFilename() << ';'
				<< node->getPos().x() << ';' << node->getPos().y() << ';' << node->getPos().z() << '\n';
		}
		file.close();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::processNodes()
{
	consolePrint(tr("processing meshes"));
	if (_modelMeshFiles->numNodes() == 0)
	{
		consolePrint(tr("aborting: nothing to process"), 2);
	}
	else
	{
		QVector3D boxGeometry = _modelMeshFiles->getGeometry();

		// sanity checks.
		for (unsigned i = 0; i < _modelMeshFiles->numNodes(); i++)
		{
			const Mesh* mesh = _modelMeshFiles->getNode(i)->getMesh();
			QVector3D meshGeometry = mesh->getMax() - mesh->getMin();

			if (	meshGeometry.x() > boxGeometry.x() or
					meshGeometry.y() > boxGeometry.y() or
					meshGeometry.z() > boxGeometry.z())
			{

				consolePrint(tr("aborted: mesh \"%1\" is too big! it's geometry is %2 while the packing Box geometry is %3.")
							 .arg(mesh->getName()).arg(toString(meshGeometry)).arg(toString(boxGeometry)), 2);
				return;
			}
		}

		consolePrint(tr("starting processing thread"));
		_modelMeshFiles->sortByBBoxSize();
		startWorker(WorkerThread::ComputePositions);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::processNodesDone()
{
	_stack->setCurrentIndex(VIEW_RESULTS);
	_progressWidget->setEnabled(false);
	_progressWidget->setValue(0);

	_actAddFile->setEnabled(true);
	_actMeshRemove->setEnabled(true);
	_actMeshScale->setEnabled(true);
	_actMeshTranslate->setEnabled(true);
	_actProcess->setEnabled(true);
	_actSaveResults->setEnabled(true);
	_actSetBoxGeometry->setEnabled(true);
	_actStop->setEnabled(false);

	consolePrint(tr("processing done in %1 milliseconds.").arg(QString::number(_threadWorker->getLastProcessingMSecs())));
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::updateWindowTitle()
{
	QVector3D userGeom = _modelMeshFiles->getGeometry() * (1 / _conversionFactor);
	QString userBoxStr = QString("(%1 %2 %3)").arg(userGeom.x()).arg(userGeom.y()).arg(userGeom.z());
	setWindowTitle(tr(APP_NAME) + tr("  box geometry: ") + userBoxStr + tr(" user units. Number of meshes: ") + QString::number(_modelMeshFiles->numNodes()));
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::consolePrint(QString str, unsigned level) const
{
	// html colors: http://www.w3schools.com/html/html_colornames.asp
	const static QString alertHtml = "<font color=\"Red\">";
	const static QString notifyHtml = "<font color=\"BlueViolet\">";
	const static QString infoHtml = "<font color=\"Black\">";
	const static QString endHtml = "</font><br>";

	QString prefix;
	switch(level)
	{
		case 0: prefix = infoHtml; break;
		case 1: prefix = notifyHtml; break;
		case 2: prefix = alertHtml; break;
		default: prefix = infoHtml; break;
	}

	QString text = QDateTime::currentDateTime().toString("%1[hh:mm:ss] %2%3").arg(prefix).arg(str).arg(endHtml);
	QTextCursor cursor = _console->textCursor();
	cursor.movePosition(QTextCursor::End);
	_console->setTextCursor(cursor);
	_console->insertHtml(text);
	/*
	QScrollBar* scrollBar = _console->verticalScrollBar();
	scrollBar->setValue(scrollBar->maximum());*/

}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::aboutThisApp()
{
	QString msg = tr("This is \"") + QString(APP_NAME) +
				  QString("\"\nby Konstantin Schlese (nulleight@gmail.com).\n") +
				  QString("Application version: %1\n").arg(qApp->applicationVersion()) +
				  QString(__DATE__) + QString(" Hamburg");
	QMessageBox::information(this, tr(APP_NAME), msg);
}
