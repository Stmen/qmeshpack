#pragma once
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QStatusBar>
#include <QMainWindow>
#include <QCoreApplication>
#include <QApplication>
#include <QFileSystemModel>
#include <QSortFilterProxyModel>
#include <QListView>
#include <QTreeView>
#include <QTableView>
#include <QListView>
#include <QSplitter>
#include <QHeaderView>
#include <QTableView>
#include <QTreeView>
#include <QInputDialog>
#include <QMessageBox>
#include <QLabel>
#include <QListView>
#include <QDockWidget>
#include <QToolBar>
#include <QTextEdit>
#include <QString>
#include <QSplitter>
#include <QStackedWidget>
#include <QProgressBar>
#include "Node.h"
#include "MeshFilesModel.h"
#include "vectorinputdialog.h"
#include "MeshPacker.h"
#include "ModelView.h"

class MainWindow : public QMainWindow
{
	Q_OBJECT
	NodeModel*		_modelMeshFiles;
	QTreeView*      _viewMeshFiles;
	ModelView*		_viewModel;
	QProgressBar*	_progressWidget;
	QToolBar*       _toolMain;
	WorkerThread*		_threadWorker;
	QTextEdit*		_console;

	QStackedWidget*	_stack;

	// actions
	QAction*			_actStop;
    QAction*			_actExit;
    QAction*			_actAddFile;
    QAction*            _actProcess;
	QAction*			_actSetBoxGeometry;	
	QAction*			_actShowResults;
	QAction*			_actSaveScreenshot;
	QAction*			_actSaveResults;
	QAction*			_actDoScaleImages;

	// specific actions that work on the current _currMeshIndex
	QModelIndex         _currMeshIndex;
	QAction*            _actMeshRemove;
	QAction*            _actMeshScale;
	QAction*            _actMeshTranslate;

	//
	float				_conversionFactor;
	unsigned			_defaultDilationValue;

	QAction*			_actSetConversionFactor;
	QAction*			_actSetDefaultDilationValue;

	void createMeshList();
    void createActions();
	void createMenusAndToolbars();
	void createStatusBar();
	void createStack();

	void startWorker(WorkerThread::Task task, QString arg = QString());

public slots:

	void dialogSetBoxGeometry();
	void dialogAddMesh();
	void dialogSetConversionFactor();
	void dialogSetDefaultDilation();
	void addMeshByName(const char* name) { _modelMeshFiles->addMesh(name, _defaultDilationValue); }
    void processNodes();
    void aboutThisApp();
	void mainNodeSelected(const QModelIndex& index);
	void mainShowResults();
    void closeEvent(QCloseEvent *event);
	void menuContextNode(const QPoint &pos);
	void processNodesDone();
	void consolePrint(QString str, unsigned level = 0) const;
	void saveScreenshot();
	void dialogSaveResults();
	void updateWindowTitle();

private:

private slots:

	void removeCurrentNode();
	void transformCurrentMesh();
	void scaleCurrentMesh();

public:
    MainWindow();
	virtual ~MainWindow();


};
