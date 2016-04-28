#include <QApplication>

#include "AppController.h"

int main(int argc, char *argv[])
{
    if (sizeof(double) != 8 || sizeof(float) != 4) {
        printf("This platform is incompatible with Optimus database format.");
        return -1;
    }

    QApplication a(argc, argv);
    ov::AppController c;
    return a.exec();
}
