#include "logsgrabber.h"
#include <QTimer>
#include <czmq.h>
#include <QMutexLocker>
#include <QJsonObject>
#include <QDebug>
#include <QJsonDocument>
#include <QUdpSocket>
#include "centralwidget.h"
#include "jsoncpp/json/reader.h"
#include <iostream>

LogsGrabber::LogsGrabber(std::shared_ptr<QThread> work_thread, float fps, QObject* parent)
    : QObject(parent), ProcLeech<CentralWidget>(work_thread, fps)
{
    qRegisterMetaType<MByteArrayPtr>("MByteArrayPtr");
    m_port = 0;
    z_sub_sock = 0;
    memset(addr_to_connect, 0x00, sizeof(addr_to_connect));
    st = -1;
}

LogsGrabber::LogsGrabber(SHP(CentralWidget) center, SHP(QThread) work_thread, float fps, QObject* parent)
    : QObject(parent), ProcLeech<CentralWidget>(work_thread, fps)
{
    m_port = 0;
    z_sub_sock = 0;
    memset(addr_to_connect, 0x00, sizeof(addr_to_connect));
    st = -1;

    buf.resize(1024);
    leech(center, fps);
}

LogsGrabber::~LogsGrabber()
{
    QMutexLocker lk(&mut);
    if (0 != z_sub_sock)
    {
        zsocket_destroy(keep.czctx(), z_sub_sock);
        z_sub_sock = 0;
    }
}

void LogsGrabber::listen(const MByteArray& srv, quint16 port)
{
    QMutexLocker lk(&mut);
    m_srv = srv.toStdString();
    if (m_srv == "localhost")
        m_srv = "127.0.0.1";

    m_port = port;
    addr = Poco::Net::SocketAddress(Poco::Net::IPAddress(m_srv), port);

    sprintf(addr_to_connect, "tcp://%s:%u", srv.data(), port);
    std::cout << "Listening: " << addr_to_connect << std::endl;

    z_sub_sock = zsocket_new(keep.czctx(), ZMQ_SUB);
    st = zsocket_connect(z_sub_sock, "%s", addr_to_connect);
    if (-1 == st)
    {
        zsocket_destroy(keep.czctx(), z_sub_sock);
        z_sub_sock = 0;
        MByteArray msg(__PRETTY_FUNCTION__);
        msg += MByteArray("FAILED zsocket_connect()");
        std::cout << msg.data() << std::endl;
        emit sig_error(msg);
    }
    else
        zmq_setsockopt (z_sub_sock, ZMQ_SUBSCRIBE, "", 0);
}

void LogsGrabber::proc()
{
    QMutexLocker lk(&mut);
    if (-1 == st || 0 == z_sub_sock)
        return;

    if (!is_attached())
        return;

    while (zsocket_poll(z_sub_sock, 40/*msec*/))
    {
        zmsg_t* msg = zmsg_recv(z_sub_sock);
        if (nullptr == msg)
            continue;

        //metadata:
        zframe_t* frame = zmsg_first(msg);
        ZConstString buf((const char*)zframe_data(frame), zframe_size(frame));
        Json::Value jo;
        bool ok = jreader.parse(buf.begin(), buf.end(), jo, false);

        //message:
        frame = zmsg_last(msg);

        if (ok && jo.type() == Json::objectValue && nullptr != jo.find(CW_KW_OBJECT.begin(), CW_KW_OBJECT.end())
                && 0 != frame)
        {
            emit message(jo, make_mbytearray(ZConstString((const char*)zframe_data(frame), zframe_size(frame))));
        }

        zmsg_destroy(&msg);
    }
}
