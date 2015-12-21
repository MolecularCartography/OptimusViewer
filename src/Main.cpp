#include <QApplication>

#include "AppController.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ov::AppController c;
    return a.exec();
}
