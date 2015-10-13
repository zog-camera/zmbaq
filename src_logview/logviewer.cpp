
#include <QtCore>
#if GPERFTOOLS_ENABLED
    #include <gperftools/profiler.h>
#endif

#include <QApplication>
#include <QtWidgets/QMainWindow>

#include <QDebug>
extern "C"
{
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
}

#include "centralwidget.h"


void term(int signum)
{
    qDebug() << "Got termination signal.";
    QApplication::quit();
}

int main(int argc, char** argv)
{
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = term;
    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGINT, &action, NULL);
    auto al = CharAllocatorSingleton::instance();

    QApplication app(argc, argv);
    auto ow = ObjectsWatchdog::instance();
    SHP(CentralWidget) widget(new CentralWidget);
    widget->show();

    return app.exec();
}
