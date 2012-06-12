#include "mainwindow.h"
#include <QProgressBar>
#include <QTableWidget>
#include <QPainter>
#include <QSettings>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QDateTime>
#include <QDebug>
#include <cstdlib>
#include <iostream>
#include <omp.h>
#include "image.h"
#include "mesh.h"
#include "GLView.h"
#include "Exception.h"

using namespace std;
#define APP_NAME "NamelessApp"
/*
 *      C
 *
 *
 *   B
 *
 *          A
 */
void testTriangle1(Image* img)
{
	QVector3D a = QVector3D(500., 100., 10);
	QVector3D b = QVector3D(100., 300., 100.);
	QVector3D c = QVector3D(800., 600., 250.);
    img->triangle(a, b, c, Image::maxValue);
}

void testTriangle2(Image* img)
{
	QVector3D a = QVector3D(100., 100., 10);
	QVector3D b = QVector3D(500., 100., 100.);
	QVector3D c = QVector3D(800., 600., 250.);
    img->triangle(a, b, c, Image::maxValue);
}

void testTriangle3(Image* img)
{
	QVector3D a = QVector3D(100., 100., 10);
	QVector3D b = QVector3D(300., 300., 100.);
	QVector3D c = QVector3D(200., 600., 250.);
    img->triangle(a, b, c, Image::maxValue);
}

void testTriangle4(Image* img)
{
	QVector3D a = QVector3D(100., 100., 10);
	QVector3D b = QVector3D(100., 300., 100.);
	QVector3D c = QVector3D(200., 100., 250.);
    img->triangle(a, b, c, Image::maxValue);
}

void testTriangle5(Image* img)
{
	QVector3D a = QVector3D(100., 100., 10);
	QVector3D b = QVector3D(300., 100., 100.);
	QVector3D c = QVector3D(300., 200., 250.);
    img->triangle(a, b, c, Image::maxValue);
}


Image* test1()
{
    Image* img = new Image(1000, 1000);
    img->clear(0);
    //img->clear(0);
    //testTriangle1(img);
    //testTriangle2(img);
    testTriangle5(img);
    return img;
}

Image* test2()
{
    Mesh mesh("/mnt/andromeda/Mesh-Dateien/cat0.off");
    return new Image(mesh, Image::Top);

}

///////////////////////////////////////////////////////////////////////////////////////////////////
MainWindow::MainWindow() : QMainWindow()
{
	QSettings settings("Konstantin", APP_NAME);
	restoreGeometry(settings.value("geometry").toByteArray());
	restoreState(settings.value("windowState").toByteArray());
	_defaultSamplesPerPixel = qvariant_cast<unsigned>(settings.value("samples_per_pixel", 10));
	_defaultDilationValue = qvariant_cast<unsigned>(settings.value("dilation", 00));

	createMeshList();

	// creating Mesh Packer	
	_threadPacker = new MeshPacker(*_modelMeshFiles, this);
	connect(_threadPacker, SIGNAL(processingDone()), this, SLOT(processNodesDone()));
	connect(_threadPacker, SIGNAL(report(QString)), this, SLOT(consolePrint(QString)));

	// create console
	QDockWidget* dockWidget = new QDockWidget(tr("Console"), this);
	_console = new QTextEdit(this);
	_console->setCurrentCharFormat(QTextCharFormat());
	dockWidget->setWidget(_console);
	addDockWidget(Qt::BottomDockWidgetArea, dockWidget);


	createActions();
	createMenus();
	mainShowDefault();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
MainWindow::~MainWindow()
{
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::createMeshList()
{
	QSettings settings("Konstantin", APP_NAME);
	QVector3D boxGeometry = qvariant_cast<QVector3D>(settings.value("box_geometry", QVector3D(1000., 1000., 1000.)));
	// create mesh list
	_modelMeshFiles = new MeshFilesModel(boxGeometry, this);
	_viewMeshFiles = new QTreeView(this);
	_viewMeshFiles->setModel(_modelMeshFiles);
	//_viewMeshFiles->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Ignored);
	_viewMeshFiles->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(_viewMeshFiles, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(viewNodeContextMenu(const QPoint &)));
	connect(_viewMeshFiles, SIGNAL(clicked(const QModelIndex &)), this, SLOT(mainNodeSelected(const QModelIndex &)));
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

	_actExit = new QAction(QIcon(), tr("&Exit"), this);
	_actExit->setShortcuts(QKeySequence::Close);
	_actExit->setStatusTip(tr("Exits this Application"));
	connect(_actExit, SIGNAL(triggered()), qApp, SLOT(quit()));

	_actAddFile = new QAction(style->standardIcon(QStyle::SP_DialogOpenButton), tr("&Add mesh"), this);
	_actAddFile->setShortcuts(QKeySequence::Open);
	_actAddFile->setStatusTip(tr("Adds a new Mesh"));
	connect(_actAddFile, SIGNAL(triggered()), this, SLOT(dialogAddMesh()));

	_actProcess = new QAction(QIcon(":images/images/reddice.png"), tr("&Process"), this);
	_actProcess->setShortcuts(QKeySequence::Refresh);
	_actProcess->setStatusTip(tr("Starts combining the Meshes."));
	connect(_actProcess, SIGNAL(triggered()), this, SLOT(processNodes()));

	_actSetBoxGeometry = new QAction(QIcon(), tr("Set &geometry"), this);
	_actSetBoxGeometry->setStatusTip(tr("Sets the bounding box geometry for the target space."));
	connect(_actSetBoxGeometry, SIGNAL(triggered()), this, SLOT(dialogSetBoxGeometry()));

	_actSetDefaultSamplesPerPixel = new QAction(QIcon(), tr("Set default &samples per pixel"), this);
	_actSetDefaultSamplesPerPixel->setStatusTip(tr("Sets the pixel rendering resolution."));
	connect(_actSetDefaultSamplesPerPixel, SIGNAL(triggered()), this, SLOT(dialogSetDefaultSamplesPerPixel()));

	_actSetDefaultDilationValue = new QAction(QIcon(), tr("Set default &dilation value"), this);
	_actSetDefaultDilationValue->setStatusTip(tr("Sets the default dilation value."));
	connect(_actSetDefaultDilationValue, SIGNAL(triggered()), this, SLOT(dialogSetDefaultDilation()));

	_actShowResults = new QAction(QIcon(), tr("Show &results"), this);
	_actShowResults->setStatusTip(tr("Shows results in the main window"));
	connect(_actShowResults, SIGNAL(triggered()), this, SLOT(mainShowResults()));

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
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::createMenus()
{
	QMenu* menu = new QMenu(tr("&File"));
	menu->insertAction(0, _actAddFile);
	menu->insertAction(0, _actProcess);
	//menu->insertAction(0, _actRmFile);
	menu->addSeparator();
	menu->insertAction(0, _actExit);
	menuBar()->addMenu(menu);

	menu = new QMenu(tr("&View"));
	menu->insertAction(0, _actShowResults);
	menuBar()->addMenu(menu);

	menu = new QMenu(tr("&Settings"));
	menu->insertAction(0, _actSetBoxGeometry);
	menu->insertAction(0, _actSetDefaultSamplesPerPixel);
	menu->insertAction(0, _actSetDefaultDilationValue);
	menuBar()->addMenu(menu);

	menu = new QMenu(tr("&Help"));
	connect(menu->addAction(tr("About QT")), SIGNAL(triggered()), qApp, SLOT(aboutQt()));
	connect(menu->addAction("About " APP_NAME), SIGNAL(triggered()), this, SLOT(aboutThisApp()));
	menuBar()->addMenu(menu);

	_toolMain = addToolBar(tr("Main toolbar"));
	_toolMain->insertAction(0, _actAddFile);
	_toolMain->insertAction(0, _actProcess);
	//_toolMain->insertAction(0, _actRmFile);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::createStatusBar()
{

}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::closeEvent(QCloseEvent *event)
{
	QSettings settings("Konstantin", APP_NAME);
	settings.setValue("geometry", saveGeometry());
	settings.setValue("windowState", saveState());
	settings.setValue("box_geometry", _modelMeshFiles->getGeometry());
	settings.setValue("samples_per_pixel", _defaultSamplesPerPixel);
	settings.setValue("dilation", _defaultDilationValue);
	QMainWindow::closeEvent(event);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::mainShowDefault()
{
	QLabel* mainWidget = new QLabel();
	mainWidget->setBackgroundRole(QPalette::Base);
	//imageLabel->setSizePolicy(QSizePolicy:: Ignored, QSizePolicy::Ignored);
	//imageLabel->setScaledContents(false);
	mainWidget->setScaledContents(false);
	Image img(1000, 700);
	testTriangle1(&img);
	img.dilate(10, Image::maxValue);


	QFont font("times", 20);
	QPixmap pixmap = QPixmap::fromImage(img.toQImage(),  Qt::ThresholdDither);
	QPainter painter(&pixmap);
	painter.setFont(font);
	painter.setPen(Qt::red);
	painter.drawText(0, 0, width(), height(), 0, "test image");
	//painter.begin( );
	// draw your image
	painter.end();

	mainWidget->setPixmap(pixmap);
	mainWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	setCentralWidget(mainWidget);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::mainShowResults()
{
	QTableWidget* tableWidget = new QTableWidget(/* rows = */ _modelMeshFiles->numNodes(), /* columns = */ 2);

	for (unsigned i = 0; i < _modelMeshFiles->numNodes(); i++)
	{
		Node* node = _modelMeshFiles->getNode(i);
		tableWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
		tableWidget->setItem(i, 0, new QTableWidgetItem(node->getMesh()->getName()));
		QVector3D pos = node->getPos();
		tableWidget->setItem(i, 1, new QTableWidgetItem(
								 QString("(%1, %2, %3)").arg(	QString::number(pos.x()),
																QString::number(pos.y()),
																QString::number(pos.z()))));

		QSplitter* splitter = new QSplitter(Qt::Vertical, this);

		splitter->addWidget(tableWidget);
		splitter->addWidget(new GLView(*_modelMeshFiles));
		setCentralWidget(splitter);
	}
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::viewNodeContextMenu(const QPoint & pos)
{
	QModelIndex idx = _viewMeshFiles->indexAt(pos);

	if(idx.isValid())
	{
		_currMeshIndex = idx;
		QMenu menu;
		menu.insertAction(0, _actMeshRemove);
		menu.insertAction(0, _actMeshScale);
		menu.insertAction(0, _actMeshTranslate);
		menu.exec(_viewMeshFiles->mapToGlobal(pos));
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::removeCurrentNode()
{
	_viewMeshFiles->clearSelection();
	_modelMeshFiles->removeRows(_currMeshIndex.row(), 1, QModelIndex());
	mainShowDefault();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::transformCurrentMesh()
{
	QString msg = tr("transforming by");
	VectorInputDialog dialog(this, msg);
	int code = dialog.exec();
	if (code == 1) // accepted
	{
		Node* node = (Node*)_modelMeshFiles->data(_currMeshIndex, Qt::UserRole).value<void*>();
		QDebug(&msg) << dialog.getResult();
		consolePrint(msg);
		node->translateMesh(dialog.getResult());
		mainNodeSelected(_currMeshIndex);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::scaleCurrentMesh()
{
	QString msg = tr("Scaling by");
	VectorInputDialog dialog(this, msg, QVector3D(1., 1., 1.));

	int code = dialog.exec();
	if (code == 1) // accepted
	{
		Node* node = (Node*)_modelMeshFiles->data(_currMeshIndex, Qt::UserRole).value<void*>();
		QDebug(&msg) << dialog.getResult();
		consolePrint(msg);
		node->scaleMesh(dialog.getResult());
		mainNodeSelected(_currMeshIndex);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::mainNodeSelected(const QModelIndex & index)
{
	Node* node = (Node*)_modelMeshFiles->data(index, Qt::UserRole).value<void*>();
	if (node)
    {
		//cout << "selected " << info->getMesh()->getName().toUtf8().constData() << endl;

        QLabel* imageLabel1 = new QLabel();
        imageLabel1->setBackgroundRole(QPalette::Base);
        //imageLabel->setSizePolicy(QSizePolicy:: Ignored, QSizePolicy::Ignored);
        //imageLabel->setScaledContents(false);
        imageLabel1->setScaledContents(true);
		imageLabel1->setPixmap(QPixmap::fromImage(node->getTop()->toQImage(), Qt::ThresholdDither));
		imageLabel1->setObjectName(node->getTop()->getName());

        QLabel* imageLabel2 = new QLabel();
        imageLabel2->setBackgroundRole(QPalette::Base);
        //imageLabel->setSizePolicy(QSizePolicy:: Ignored, QSizePolicy::Ignored);
        //imageLabel->setScaledContents(false);
        imageLabel2->setScaledContents(true);
		imageLabel2->setPixmap(QPixmap::fromImage(node->getBottom()->toQImage(), Qt::ThresholdDither));
		imageLabel2->setObjectName(node->getBottom()->getName());

		QWidget* widget = new QWidget;
		QGridLayout* layout = new QGridLayout;
		widget->setLayout(layout);

		layout->addWidget(imageLabel1, 0, 0);
		layout->addWidget(imageLabel2, 1, 0);
		layout->addWidget(new GLView(node, _modelMeshFiles->getGeometry()), 0, 1, 2, 1);
		widget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

		setCentralWidget(widget);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::dialogSetBoxGeometry()
{
	QString msg = tr("Setting box geometry to ");
	VectorInputDialog dialog(this, msg, _modelMeshFiles->getGeometry());

	int code = dialog.exec();
	if (code == 1) // accepted
	{
		QVector3D result = floor(dialog.getResult());
		if (result.x() > 0 and result.y() > 0 and result.z() > 0)
		{
			QDebug(&msg) << dialog.getResult();
			consolePrint(msg);
			_modelMeshFiles->setGeometry(dialog.getResult());
		}
		else
			consolePrint(tr("Bad input"), 2);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::dialogSetDefaultSamplesPerPixel()
{
	QString msg = tr("Setting default samples per pixel to ");
	bool ok;
	double resolution = QInputDialog::getInteger(this, msg, tr("new resolution in real pixels"), _defaultSamplesPerPixel,
												 0, 1000, 1, &ok);
	if (ok)
	{
		_defaultSamplesPerPixel = resolution;
		consolePrint(msg + QString::number(resolution));
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


/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::dialogAddMesh()
{
    QString filename = QFileDialog::getOpenFileName(this,
        tr("Open Mesh File"), "", tr("OFF files (*.off *.OFF);;All files (*.*)"));

    if (not filename.isEmpty())
    {
		QString msg = QString(tr("Failed to open file \"")) + filename + QString("\" ");
        try
        {
			consolePrint(QString("opening \"%1\"").arg(filename));
			_modelMeshFiles->addMesh(filename.toUtf8().constData(), _defaultSamplesPerPixel, _defaultDilationValue);
        }
		catch (const std::exception& ex)
        {
            msg += QString(ex.what());
			consolePrint(msg, 2);
			QMessageBox::critical(this, tr("Open failed"), msg);
        }
        catch (...)
        {
			consolePrint(msg, 2);
			QMessageBox::critical(this, tr("Open failed"), msg);
        }
    }

}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::consolePrint(QString str, unsigned level)
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

	QString text = QDateTime::currentDateTime().toString("%1[hh:mm:ss] %2%3").arg(prefix, str, endHtml);
	QTextCursor cursor = _console->textCursor();
	_console->insertHtml(text);
	_console->setTextCursor(cursor);
	// TODO check if console too long and remove the lines above.
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::processNodes()
{
	if (_threadPacker->isRunning())
	{
		consolePrint(tr("processing thread is still running, aborting"));
		return;
	}

	consolePrint(tr("processing meshes"));

	if (_modelMeshFiles->numNodes() != 0)
	{		
		QVector3D boxGeometry = _modelMeshFiles->getGeometry();
		for (unsigned i = 0; i < _modelMeshFiles->numNodes(); i++)
		{
			const Mesh* mesh = _modelMeshFiles->getNode(i)->getMesh();
			QVector3D meshGeometry = mesh->getMax() - mesh->getMin();

			if (	meshGeometry.x() > boxGeometry.x() or
					meshGeometry.y() > boxGeometry.y() or
					meshGeometry.z() > boxGeometry.z())
			{

				consolePrint(tr("aborted: mesh \"%1\" is too big! it's geometry is (%2, %3, %4) while the packing Box geometry is (%5, %6, %7).")
							 .arg(mesh->getName(),
								  QString::number(meshGeometry.x()),
								  QString::number(meshGeometry.y()),
								  QString::number(meshGeometry.z()),
								  QString::number(boxGeometry.x()),
								  QString::number(boxGeometry.y()),
								  QString::number(boxGeometry.z())), 2);
				return;
			}
		}

		QProgressBar* progressBar = new QProgressBar();
		progressBar->setRange(0, _threadPacker->maxProgress());
		connect(_threadPacker, SIGNAL(reportProgress(int)), progressBar, SLOT(setValue(int)));

		QRect rect = centralWidget()->geometry();
		progressBar->setGeometry(rect);
		setCentralWidget(progressBar);

		consolePrint(tr("starting processing thread"));		
		_threadPacker->start();
	}
	else
	{
		mainShowDefault();
		consolePrint(tr("aborted: nothing to process"), 2);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::processNodesDone()
{
	mainShowResults();
	consolePrint("done");
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::aboutThisApp()
{
    const char* msg = "This is \"" APP_NAME "\" by Konstantin Schlese, nulleight@gmail.com";
    QMessageBox::information(this, tr(APP_NAME), tr(msg));
}



