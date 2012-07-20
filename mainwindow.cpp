#include "mainwindow.h"
#include <QMessageBox>
#include <QCoreApplication>
#include <QApplication>
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
#include "Exception.h"
#include "vectorinputdialog.h"
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
void drawIntro2(Image* img, QVector3D offset, double from, double r1, double r2)
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
		img->drawTriangle(a, b, c, Image::x_greater_y);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void drawIntro(Image* img, QVector3D offset, double from, double r1, double r2)
{
	QVector3D a(100, 100, 100), b(400, 100, 200), c(400, 700, 300);
	img->drawTriangle(a, b, c, Image::x_greater_y);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
MainWindow::MainWindow() :
	QMainWindow(),
	_modelMeshFiles(this),
	_stack(new QStackedWidget(this))
{
    _execAfterWorkerFinished = [](){};
    QSettings settings(APP_VENDOR, APP_NAME);
	restoreGeometry(settings.value("geometry").toByteArray());
	restoreState(settings.value("windowState").toByteArray());
	_conversionFactor = qvariant_cast<float>(settings.value("conversionFactor", 1.));
	unsigned dilation = qvariant_cast<unsigned>(settings.value("dilation", 00));
	_modelMeshFiles.setDefaultDilationValue(dilation);

	QGLFormat fmt;
	fmt.setAlpha(true);
	fmt.setStereo(false);
	fmt.setSampleBuffers(true);
	QGLFormat::setDefaultFormat(fmt);

	setCentralWidget(_stack);
	QVector3D box_geom = qvariant_cast<QVector3D>(settings.value("box_geometry", QVector3D(1000., 1000., 1000.)));
	_modelMeshFiles.setGeometry(box_geom);
	_threadWorker = new WorkerThread(this, _modelMeshFiles);

	createMeshList();

	// create console
	_console = new Console(this);
	QDockWidget* dockWidget = new QDockWidget(tr("Console"), this);	
	dockWidget->setWidget(_console);
	addDockWidget(Qt::BottomDockWidgetArea, dockWidget);

	createStack();	
	createActions();
	createMenusAndToolbars(); // menus depend on existing actions and some widgets like _progress*

	connect(&_modelMeshFiles, SIGNAL(dataChanged(QModelIndex,QModelIndex)), _viewBox, SLOT(update()), Qt::QueuedConnection);
	connect(&_modelMeshFiles, SIGNAL(geometryChanged()), this, SLOT(updateWindowTitle()));
	connect(&_modelMeshFiles, SIGNAL(numNodesChanged()), this, SLOT(updateWindowTitle()));	
	connect(_threadWorker, SIGNAL(processingDone()), _viewBox, SLOT(update()), Qt::QueuedConnection);
    connect(_threadWorker, SIGNAL(reportProgressMax(int)), _progressWidget, SLOT(setMaximum(int)), Qt::QueuedConnection);
	connect(_threadWorker, SIGNAL(reportProgress(int)), _progressWidget, SLOT(setValue(int)), Qt::QueuedConnection);
	connect(_threadWorker, SIGNAL(processingDone()), this, SLOT(processNodesDone()), Qt::QueuedConnection);
	connect(_threadWorker, SIGNAL(report(QString,Console::InfoLevel)), _console, SLOT(addInfo(QString,Console::InfoLevel)), Qt::QueuedConnection);
	setWindowIcon(QIcon(":images/images/logo.png"));
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
		tr("All supported files (*.png *.jpg);; PNG (*.png);; JPG (*.jpg)"));

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
	settings.setValue("conversionFactor", _conversionFactor);
	settings.setValue("dilation", _modelMeshFiles.getDefaultDilationValue());
	QMainWindow::closeEvent(event);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::createMeshList()
{
	_viewMeshFiles = new QTreeView(this);
	_viewMeshFiles->setModel(&_modelMeshFiles);
	_viewMeshFiles->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	_viewMeshFiles->header()->setSortIndicatorShown(true); // optional
	_viewMeshFiles->header()->setClickable(true);
	_viewMeshFiles->setSortingEnabled(true);
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
    QSettings settings(APP_VENDOR, APP_NAME);
	QStyle* style = QApplication::style();

	_actStop = new QAction(QIcon(":/trolltech/styles/commonstyle/images/stop-32.png"), tr("&Stop"), this);
	_actStop->setEnabled(false);
    connect(_actStop, SIGNAL(triggered()), _threadWorker, SLOT(shouldStop()), Qt::QueuedConnection);

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

	_actClear = new QAction(QIcon(":/trolltech/styles/commonstyle/images/standardbutton-clear-32.png"), tr("&Clear"), this);
	_actClear->setStatusTip(tr("Removes all meshes."));
	connect(_actClear, SIGNAL(triggered()), &_modelMeshFiles, SLOT(clear()));
	connect(_actClear,  SIGNAL(triggered()), _viewBox, SLOT(update()));

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

	_actMeshScale = new QAction(QIcon(), tr("&Scale"), this);
	_actMeshScale->setStatusTip(tr("Scale this mesh"));
	connect(_actMeshScale, SIGNAL(triggered()), this, SLOT(scaleCurrentMesh()));

    _actToggleScaleImages = new QAction(QIcon(), tr("Scale &images"), this);
    _actToggleScaleImages->setStatusTip(tr("Scale images"));
    _actToggleScaleImages->setCheckable(true);
    connect(_actToggleScaleImages, SIGNAL(toggled(bool)), _viewModel, SLOT(setScaleImages(bool)));
    bool doScaleImages = settings.value("scale_images", false).toBool();
    //_viewModel->setScaleImages(doScaleImages);
    _actToggleScaleImages->setChecked(doScaleImages);

    _actToggleUseLighting = new QAction(QIcon(), tr("Use &lighting"), this);
    _actToggleUseLighting->setStatusTip(tr("Uses lighting"));
    _actToggleUseLighting->setCheckable(true);
	bool doUseLighting = settings.value("use_lighting", true).toBool();
	_actToggleUseLighting->setChecked(doUseLighting);

	// conecting after setChecked to avoid launching a thread while there are no nodes
    connect(_actToggleUseLighting, SIGNAL(toggled(bool)), this, SLOT(setLighting(bool)));    
    //setLighting(doUseLighting);
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
    menu->insertAction(0, _actToggleScaleImages);
    menu->insertAction(0, _actToggleUseLighting);
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
	_toolMain->insertAction(0, _actClear);
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
	label->setScaledContents(true);
	label->setBackgroundRole(QPalette::Base);
	label->setSizePolicy(QSizePolicy:: Ignored, QSizePolicy::Ignored);
	//label->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

#ifdef TEST_IMAGE
	Image img(700, 700);
	//testTriangle1(&img);
	//img.dilate(10, Image::maxValue);
	drawIntro(&img, QVector3D(img.getWidth() / 2, img.getHeight() / 2, 0), 0, 150, 300);
	Image* o = img.clockwizeRotate90(3);
	//Image* o = new Image(img);
	//drawIntro(&img, QVector3D(img.getWidth() / 2, img.getHeight() / 2, 0), 0.2, 50, 300);
	//drawIntro(&img, QVector3D(img.getWidth() / 2, img.getHeight() / 2, 0), 0.4, 50, 300);

	QFont font("times", 20);
	QPixmap pixmap = QPixmap::fromImage(o->toQImage(),  Qt::ThresholdDither);
	QPainter painter(&pixmap);
	painter.setFont(font);
	painter.setPen(Qt::red);
	painter.drawText(0, 0, width(), height(), 0, APP_NAME);
	painter.end();
	label->setPixmap(pixmap);
	label->setScaledContents(false);
#else
	QPixmap pixmap(":images/images/logo.png");
	label->setPixmap(pixmap);
#endif
	_stack->addWidget(label);

    QSettings settings(APP_VENDOR, APP_NAME);
    bool use_lighting = settings.value("use_lighting", true).toBool();
	_viewBox = new GLView(&_modelMeshFiles, use_lighting);
    _stack->addWidget(_viewBox);

    _viewModel = new ModelView (use_lighting, this);
	_stack->addWidget(_viewModel);

	_progressWidget = new QProgressBar();
	_progressWidget->setEnabled(false);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::mainNodeSelected(const QModelIndex & index)
{
	if (index.isValid())
	{
		Node* node = (Node*)_modelMeshFiles.data(index, Qt::UserRole).value<void*>();
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
		_console->addInfo(tr("removing node \"%1\"").arg(_modelMeshFiles.getNode(idx)->getMesh()->getName()), Console::Info);
		_modelMeshFiles.removeRows(idx, 1, QModelIndex());
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
		Node* node = (Node*)_modelMeshFiles.data(_currMeshIndex, Qt::UserRole).value<void*>();
		msg += QString("(%1 %2 %3)").arg(dialog.getResult().x()).arg(dialog.getResult().y()).arg(dialog.getResult().z());
		_console->addInfo(msg, Console::Info);
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
										 ), _modelMeshFiles.getGeometry() * invConv);

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
			_console->addInfo(tr("Setting box geometry to %1 application units.").arg(resultStr), Console::Info);
			_modelMeshFiles.setGeometry(result);
            QSettings settings(APP_VENDOR, APP_NAME);
            settings.setValue("box_geometry", result);
		}
		else
			_console->addInfo(tr("Bad input"), Console::Error);
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
		_console->addInfo(tr("Setting new conversion factor to %1").arg(f));
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::dialogSetDefaultDilation()
{
	QString msg = tr("Setting default dilation value ");
	bool ok;
	double value = QInputDialog::getInteger(this, msg, tr("dilation value"), _modelMeshFiles.getDefaultDilationValue(),
												 0, 80, 1, &ok);
	if (ok)
	{
		_modelMeshFiles.setDefaultDilationValue(value);
		_console->addInfo(msg + QString::number(value));
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
static void unrollListFiles(QString filename, QStringList& out)
{
	QFile file(filename);
	if (not file.open(QIODevice::ReadOnly | QIODevice::Text))
		THROW(BaseException, QString("Unable to open file %1: %2").arg(filename, file.errorString()));


	while (not file.atEnd() and file.isReadable())
	{		
		QString str = QString(file.readLine()).trimmed();
		if (str.isEmpty()) // ignore unparsable
			continue;
		else
			out.append(str);
	}
	file.close();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::dialogAddMesh()
{
	QFileDialog dialog(this,
		  /* caption = */ tr("Save results as"),
		  /* directory = */ "",
		  "OFF meshes (*.off *.OFF);;"
		  "TXT mesh list(*.txt *.TXT);;"
		  "All supported files (*.off *.txt)");

	dialog.setFileMode(QFileDialog::ExistingFiles);
	if (not dialog.exec())
		return;

	QStringList filenames = dialog.selectedFiles();
	if (filenames.isEmpty())
		return;

	QStringList out;
	for(long i = 0; i < filenames.size(); ++i)
	{
		if (filenames[i].endsWith(".txt") or filenames[i].endsWith(".TXT"))
			unrollListFiles(filenames[i], out);
		else
			out.append(filenames[i]);
	}

	_console->addInfo(tr("loading a list of meshes."));
	startWorker(WorkerThread::LoadMeshList, QVariant(out));
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::startWorker(WorkerThread::Task task, QVariant arg)
{
	_actAddFile->setEnabled(false);
	_actMeshRemove->setEnabled(false);
	_actMeshScale->setEnabled(false);
	_actProcess->setEnabled(false);
	_actSaveResults->setEnabled(false);
	_actSetBoxGeometry->setEnabled(false);
	_actClear->setEnabled(false);
	_actStop->setEnabled(true);	
    _actToggleUseLighting->setEnabled(false);

	_threadWorker->setArgument(arg);
	_threadWorker->setTask(task);
	_progressWidget->setEnabled(true);
	_threadWorker->start();
}

#include <QDir>
/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::dialogSaveResults()
{
	if(_modelMeshFiles.numNodes() == 0)
	{
		_console->addInfo(tr("no nodes to save"), Console::Error);
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
		_console->addInfo(tr("saving results to an OFF mesh \"%1\"").arg(filename));
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

		_console->addInfo(tr("saving results to a list \"%1\"").arg(filename));
		QTextStream out(&file);

		for (unsigned i = 0; i < _modelMeshFiles.numNodes(); i++)
		{
			Node* node = _modelMeshFiles.getNode(i);
            out << node->getMesh()->getFilename() << ';'
				<< node->getPos().x() << ';' << node->getPos().y() << ';' << node->getPos().z() << '\n';
		}
		file.close();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::processNodes()
{
	_console->addInfo(tr("processing meshes"));
	if (_modelMeshFiles.numNodes() == 0)
	{
		_console->addInfo(tr("aborting: nothing to process"), Console::Error);
	}
	else
	{
		QVector3D boxGeometry = _modelMeshFiles.getGeometry();

		// sanity checks.
		for (unsigned i = 0; i < _modelMeshFiles.numNodes(); i++)
		{
			const Mesh* mesh = _modelMeshFiles.getNode(i)->getMesh();
			QVector3D meshGeometry = mesh->getMax() - mesh->getMin();

			if (	meshGeometry.x() > boxGeometry.x() or
					meshGeometry.y() > boxGeometry.y() or
					meshGeometry.z() > boxGeometry.z())
			{

				_console->addInfo(tr("aborted: mesh \"%1\" is too big! it's geometry is %2 while the packing Box geometry is %3.")
								  .arg(mesh->getName()).arg(toString(meshGeometry)).arg(toString(boxGeometry)), Console::Error);
				return;
			}
		}

		_console->addInfo(tr("starting processing thread"));
		_modelMeshFiles.sortByBBoxSize();
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
	_actProcess->setEnabled(true);
	_actSaveResults->setEnabled(true);
	_actSetBoxGeometry->setEnabled(true);
	_actClear->setEnabled(true);
	_actStop->setEnabled(false);
    _actToggleUseLighting->setEnabled(true);
	_console->addInfo(tr("processing done in %1 milliseconds.").arg(QString::number(_threadWorker->getLastProcessingMSecs())), Console::Notify);
    _execAfterWorkerFinished();
    _execAfterWorkerFinished = [](){};
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::updateWindowTitle()
{
	QVector3D userGeom = _modelMeshFiles.getGeometry() * (1 / _conversionFactor);
	QString userBoxStr = QString("(%1 %2 %3)").arg(userGeom.x()).arg(userGeom.y()).arg(userGeom.z());
	setWindowTitle(tr(APP_NAME) + tr("  box geometry: ") + userBoxStr + tr(" user units. Number of meshes: ") +
				   QString::number(_modelMeshFiles.numNodes()));
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::aboutThisApp()
{
	QString ver = qApp->applicationVersion();
	if (ver.at(ver.size() - 2) == 'B')
		ver = ver + "-beta" + ver.at(ver.size() - 1);

	QString msg = tr("This is \"") + QString(APP_NAME) +
				  QString("\"\nby Konstantin Schlese (nulleight@gmail.com).\n") +
				  QString("Application version: %1\n").arg(ver) +
				  QString(__DATE__) + QString(" Hamburg");
	QMessageBox::information(this, tr(APP_NAME), msg);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::setLighting(bool lighting_enable)
{
    if (lighting_enable)
    {
        _execAfterWorkerFinished = [this]()
        {
            _viewBox->setUseLighting(true);
            _viewModel->getGLView()->setUseLighting(true);

            QSettings settings(APP_VENDOR, APP_NAME);
            settings.setValue("use_lighting", true);
            _viewBox->update();
            _viewModel->getGLView()->update();
        };
        startWorker(WorkerThread::MakeNormals);
    }
    else
    {
        _viewBox->setUseLighting(false);
        _viewModel->getGLView()->setUseLighting(false);

        QSettings settings(APP_VENDOR, APP_NAME);
        settings.setValue("use_lighting", false);
        _viewBox->update();
        _viewModel->getGLView()->update();
    }

}
