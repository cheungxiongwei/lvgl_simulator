#include <QApplication>
#include "main.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    LVGLWidget w;
    w.show();
    return app.exec();
}
