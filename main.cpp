#include "mainwindow.h"
#include <QApplication>
#include "util.h"


int main(int argc, char** argv)
{
	//test_iterators();

	QApplication app(argc, argv);
	app.setApplicationVersion(APP_VERSION);
	app.setStyleSheet("QSplitter::handle { background-color: gray }");
	MainWindow win;
	win.show();
    int result = app.exec();
    #ifdef WIN32
    // this is an ugly hack because qt doesn't work with openmp in it's QThreads (4.8.2 + gcc4.7) and to fix the issue I had to forcefully
    // terminate processing thread in qthread_win.cpp (QtGui4.dll) since openmp still thinks that destroyed QThread still lives. Since the
    // worker thread is forcefully terminated, ms visual c++ runtime is not properly notified and waits for completion forever.
    TerminateProcess(GetCurrentProcess(), 0);
    #endif
    return result;
}
