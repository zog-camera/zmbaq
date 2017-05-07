#ifndef LOGVIEWER_H
#define LOGVIEWER_H
#include <QObject>
#include <QApplication>
#include <QDebug>
#include "mbytearray.h"
#include "genericstreamwriter.h"


class ErrorCheck : public QObject
{
    Q_OBJECT

public:
    ErrorCheck(QObject* parent = 0) : QObject(parent)
    {

    }

public slots:
    void on_writer_error(std::shared_ptr<ZMBCommon::GenericStreamWriter> writer,
                         std::shared_ptr<std::string> msg)
    {
        qDebug() << "Error occured: " << msg->data();
        QApplication::quit();
    }

};

#endif // LOGVIEWER_H

