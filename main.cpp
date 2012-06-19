#include "mainwindow.h"

int main(int argc, char** argv)
{    
	QApplication app(argc, argv);
	app.setStyleSheet("QSplitter::handle { background-color: gray }");
	MainWindow win;
	win.show();
	return app.exec();
}
