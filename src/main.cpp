#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("Crofteq");
    QCoreApplication::setOrganizationDomain("crofteq.se");
    QCoreApplication::setApplicationName("Ordning");

    QApplication app(argc, argv);
    MainWindow w;
    w.show();
    return app.exec();
}
