#include "IV_Plotter.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	IV_Plotter w;
	w.show();
	return a.exec();
}
