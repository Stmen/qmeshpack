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
#include "Node.h"
#include "MeshFilesModel.h"
#include "vectorinputdialog.h"
#include "MeshPacker.h"
#include "PackBox.h"

class MainWindow : public QMainWindow
{
	Q_OBJECT
	MeshFilesModel* _modelMeshFiles;
	QTreeView*      _viewMeshFiles;
	QToolBar*       _toolMain;
	MeshPacker*		_threadPacker;
	QTextEdit*		_console;
	QWidget*		_centralWidget;


	// actions
    QAction*			_actExit;
    QAction*			_actAddFile;
    QAction*            _actProcess;
	QAction*			_actSetBoxGeometry;
	QAction*			_actSetDefaultSamplesPerPixel;
	QAction*			_actShowResults;

	// specific actions that work on the current _currMeshIndex
	QModelIndex         _currMeshIndex;
	QAction*            _actMeshRemove;
	QAction*            _actMeshScale;
	QAction*            _actMeshTranslate;
	unsigned			_defaultSamplesPerPixel;

	void createMeshList();
    void createActions();
	void createMenus();
	void createStatusBar();

public slots:

	void dialogSetBoxGeometry();
	void dialogAddMesh();
	void dialogSetDefaultSamplesPerPixel();
	void addMeshByName(const char* name) { _modelMeshFiles->addMesh(name); }
    void processNodes();
    void aboutThisApp();
    void nodeSelected(const QModelIndex& index);
    void closeEvent(QCloseEvent *event);
	void viewNodeContextMenu(const QPoint &pos);
	void processNodesDone();
	void consolePrint(QString str, unsigned level = 0);
	void mainShowDefault();
	void mainShowResults();

private:



private slots:

	void removeCurrentNode();
	void transformCurrentMesh();
	void scaleCurrentMesh();


public:
    MainWindow();
	virtual ~MainWindow();


};
