#include <QApplication>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QApplication::setApplicationVersion("0.1.0");

    MainWindow *w = new MainWindow();
    w->show();
    return a.exec();
}
