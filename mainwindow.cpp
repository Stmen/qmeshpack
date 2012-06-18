#include "mainwindow.h"
#include <QProgressBar>
#include <QTableWidget>
#include <QPainter>
#include <QSettings>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QDateTime>
#include <QDebug>
#include <QImage>
#include <cstdlib>
#include <iostream>
#include <omp.h>
#include "image.h"
#include "mesh.h"
#include "GLView.h"
#include "Exception.h"
#include "config.h"
using namespace std;

#define VIEW_WELCOME 0
#define VIEW_RESULTS 1
#define VIEW_MODEL 2
#define VIEW_PROGRESS 3

void drawIntro(Image* img, QVector3D offset, double from, double r1, double r2)
{
	for (double i = from; i < from + M_PI * 2; i += 0.8)
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

///////////////////////////////////////////////////////////////////////////////////////////////////
MainWindow::MainWindow() : QMainWindow()
{
	QSettings settings("Konstantin", APP_NAME);
	restoreGeometry(settings.value("geometry").toByteArray());
	restoreState(settings.value("windowState").toByteArray());
	_defaultSamplesPerPixel = qvariant_cast<unsigned>(settings.value("samples_per_pixel", 10));
	_defaultDilationValue = qvariant_cast<unsigned>(settings.value("dilation", 00));

	_stack = new QStackedWidget(this);
	setCentralWidget(_stack);

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
	createStack();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
MainWindow::~MainWindow()
{
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::saveScreenshot()
{
	QString filename = QFileDialog::getSaveFileName(this,
		tr("Save main window screenshot"), "",
		tr("PNG (*.png);; JPG (*.jpg);; All Files (*)"));

	if (not filename.isEmpty())
	{
		QStringList l = filename.split(".");
		QString ext = l.at(l.size() - 1).toLower();

		if (ext != "jpg" or ext != "png" or ext != "bmp")
			filename += ".png";

		QImage img(size(), QImage::Format_RGB32);
		QPainter painter(&img);
		render(&painter);
		img.save(filename);
	}
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
	connect(_viewMeshFiles, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(menuContextNode(const QPoint &)));
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
	//_toolMain->insertAction(0, _actSaveScreenshot);
	_toolMain->insertAction(0, _actSaveResults);
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
	label->setScaledContents(false);
	Image img(700, 700);
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

	_stack->addWidget(new GLView(_modelMeshFiles));
	_viewModel = new ModelView(this);
	_stack->addWidget(_viewModel);

	_progressWidget = new QProgressBar();
	connect(_threadPacker, SIGNAL(reportProgress(int)), _progressWidget, SLOT(setValue(int)));
	_stack->addWidget(_progressWidget);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::mainNodeSelected(const QModelIndex & index)
{
	Node* node = (Node*)_modelMeshFiles->data(index, Qt::UserRole).value<void*>();
	_viewModel->setNode(node, _modelMeshFiles->getGeometry());
	_stack->setCurrentIndex(VIEW_MODEL);
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
	_viewMeshFiles->clearSelection();
	_stack->setCurrentIndex(VIEW_RESULTS);
	_modelMeshFiles->removeRows(_currMeshIndex.row(), 1, QModelIndex());
	createStack();
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
	VectorInputDialog dialog(this, msg, QVector3D(1., 1., 1.));

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
			_viewModel->getGLView()->getCurrentModel()->setGeometry(dialog.getResult());
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
	double resolution = QInputDialog::getInteger(this, msg,
												 tr("new resolution in real pixels"),
												 _defaultSamplesPerPixel,
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

#define EXT_DOTTED "."RESULTS_APPEND_EXTENSION
/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::dialogAddMesh()
{
    QString filename = QFileDialog::getOpenFileName(this,
		tr("Open Mesh File"), "", tr(
#ifdef RESULTS_APPEND_EXTENSION
		"All supported files (*.off *.OFF *"EXT_DOTTED");; "
		"Mesh file list (*"EXT_DOTTED");; "
#endif
		"OFF files (*.off *.OFF);; "
		"All Files (*.*)"));

    if (not filename.isEmpty())
    {
		QString msg = QString(tr("Failed to open file \"")) + filename + QString("\" ");
        try
        {
			consolePrint(QString("opening \"%1\"").arg(filename));
			QStringList l = filename.split(".");
			QString ext = l.at(l.size() - 1).toLower();
			if (ext == "off")
			{
				_modelMeshFiles->addMesh(filename.toUtf8().constData(), _defaultSamplesPerPixel, _defaultDilationValue);
			}
#ifdef RESULTS_APPEND_EXTENSION
			else if (ext == RESULTS_APPEND_EXTENSION)
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
					Node* node = _modelMeshFiles->addMesh(off_filename.toUtf8().constData(),
														  _defaultSamplesPerPixel, _defaultDilationValue);

					double x = list.at(1).toDouble(), y = list.at(2).toDouble(), z = list.at(3).toDouble();
					node->setPos(QVector3D(x, y, z));
				}
			}
#endif
			else
				throw Exception("Unknown file extension.");
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
void MainWindow::dialogSaveResults()
{
	QString filename = QFileDialog::getSaveFileName(this,
		tr("Save main window screenshot"), "", tr(
#ifdef RESULTS_APPEND_EXTENSION
		"Mesh file list (*"EXT_DOTTED");; "
#endif
		"All Files (*.*)"));

	if (not filename.isEmpty())
	{

#ifdef RESULTS_APPEND_EXTENSION
		QStringList l = filename.split(".");
		QString ext = l.at(l.size() - 1).toLower();

		if (ext != RESULTS_APPEND_EXTENSION )
			filename += EXT_DOTTED;
#endif

	   QFile file(filename);
	   if (not file.open(QIODevice::WriteOnly | QIODevice::Text))
	   {
		   QMessageBox::information(this, tr("Unable to open file"), file.errorString());
		   return;
	   }

	   consolePrint(QString("saving results to \"%1\"").arg(filename));
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
	if (_threadPacker->isRunning())
	{
		consolePrint(tr("processing thread is still running, aborting"));
		return;
	}

	consolePrint(tr("processing meshes"));

	if (_modelMeshFiles->numNodes() != 0)
	{		
		_stack->setCurrentIndex(VIEW_PROGRESS);
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

		_progressWidget->setRange(0, _threadPacker->maxProgress());		

		consolePrint(tr("starting processing thread"));		
		_threadPacker->start();
	}
	else
	{
		consolePrint(tr("aborted: nothing to process"), 2);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::processNodesDone()
{
	_stack->setCurrentIndex(VIEW_RESULTS);
	consolePrint("done");
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

	QString text = QDateTime::currentDateTime().toString("%1[hh:mm:ss] %2%3").arg(prefix).arg(str).arg(endHtml);
	QTextCursor cursor = _console->textCursor();
	cursor.movePosition(QTextCursor::End);
	_console->insertHtml(text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::aboutThisApp()
{
    const char* msg = "This is \"" APP_NAME "\" by Konstantin Schlese, nulleight@gmail.com";
    QMessageBox::information(this, tr(APP_NAME), tr(msg));
}
