#include "mainwindow.h"

int main(int argc, char** argv)
{    
	QApplication app(argc, argv);
	MainWindow win;
	for (int i = 1; i < argc; i++)
		win.addMeshByName(argv[i]);

	win.show();
	return app.exec();
}
