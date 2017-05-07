#ifndef UDPLOGSGRABBER_H
#define UDPLOGSGRABBER_H
#include <QByteArray>
#include <QObject>
#include <QMutex>
#include <QUdpSocket>
#include "zmq_ctx_keeper.h"
#include <Poco/Net/DatagramSocket.h>

#include "zmbaq_common.h"
#include "procleech.hpp"

class QTimer;
class CentralWidget;

ZM_DEF_STATIC_CONST_STRING(CW_KW_OBJECT, "object")

/**---------- Listens UDP broadcast -----------------*/
class LogsGrabber : public QObject,
        public ProcLeech</*victim*/CentralWidget>
{
    Q_OBJECT

public:

    explicit LogsGrabber(SHP(QThread) work_thread, float fps = 40.0f, QObject* parent = 0);
    explicit LogsGrabber(SHP(CentralWidget) center, SHP(QThread) work_thread, float fps = 40.0f, QObject* parent = 0);
    virtual ~LogsGrabber();

signals:
    void message(Json::Value metadata, std::stringPtr msg);
    void sig_error(std::string err);


public slots:

    void listen(const std::string& srv, quint16 port);

    void proc();

private:
    std::string name;
    std::string buf;

    Json::Reader jreader;

    //--- ZMQ SUB ---
    zmq_ctx_keeper keep;
    void* z_sub_sock;
    char addr_to_connect[80];
    int st;
    //---------------

    Poco::Net::SocketAddress addr;
    QMutex mut;
    std::string m_srv;
    quint16 m_port;


};

#endif // UDPLOGSGRABBER_H
