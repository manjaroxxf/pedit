#include <QApplication>

#include "confighelper.h"
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    ConfigHelper *cfHelper = new ConfigHelper;

    MainWindow w(cfHelper);
    w.show();
    return a.exec();
}
