/*A video surveillance software with support of H264 video sources.
Copyright (C) 2015 Bogdan Maslowsky, Alexander Sorvilov. 

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.*/


#include "genericstreamreader.h"
//#include <QLocalSocket>
//#include <QUdpSocket>
//#include <QTcpSocket>
#include <Poco/Net/DatagramSocket.h>
#include <Poco/Net/StreamSocket.h>
#include <iostream>
#include "zmq_ctx_keeper.h"
#include "logservice.h"

namespace ZMBCommon {

class GSRPriv : public std::enable_shared_from_this<GSRPriv>
{
public:

    enum SP_TYPE {SP_NONE = 0x00, SP_UDP, SP_TCP, SP_LOCAL/*, SP_ZMG_SUB*/};

    std::shared_ptr<GSRPriv> this_ptr()
    {
        return shared_from_this();
    }

    GSRPriv(GenericStreamReader* parent) : daddy(parent),
        sock_type(SP_NONE),
        server(GSW_VAL_LOCALHOST.begin()),
        port(9123)
    {
//        unix_sock = nullptr;
    }
    ~GSRPriv()
    {
    }

    bool jparse(const Json::Value& config)
    {
        //find "socket":
        if (nullptr == config.find(GSW_KW_SOCKET.begin(), GSW_KW_SOCKET.end()))
        {
            LERROR(ZCSTR("GenericReader"), ZCSTR("Error in parsing config: socket keyword not found."));
            LERROR(ZCSTR("GenericReader"), config);
            Json::FastWriter wr;
            std::cout << wr.write(config);
            return false;
        }

        MByteArray pr;
        pr += config[GSW_KW_SOCKET.begin()].asCString();
        if (GSW_UDP == pr)
           sock_type = SP_UDP;

        if (GSW_TCP == pr)
           sock_type = SP_TCP;

        bool has_error = true;
        if (sock_type != SP_LOCAL && sock_type != SP_NONE)
        {
            has_error = !gsw_jparse_server_port(&server, &port, &config);
        }
        else
        if (GSW_KW_AF_UNIX == pr)
        {
            sock_type = SP_LOCAL;
            const Json::Value* jpdesc = 0;
            if (GSW_KW_DESCRIPTOR.is_key_of(&config, &jpdesc))
            {
                server = jpdesc->asCString();
                has_error = false;
            }
        }
        addr = Poco::Net::SocketAddress(Poco::Net::IPAddress(server), port);

        try {
            switch (sock_type) {
            case SP_UDP:
                udp_sock = std::make_shared<Poco::Net::DatagramSocket>();
                udp_sock->setBlocking(false);
                udp_sock->bind(addr);
                break;
            case SP_TCP:
                tcp_sock = std::make_shared<Poco::Net::StreamSocket>();
                tcp_sock->connect(addr);
                break;

            case SP_LOCAL:
//                if (nullptr != unix_sock)
                {
//                    if (QThread::currentThread() == unix_sock->thread())
//                        unix_sock->close();
//                    unix_sock->deleteLater();
//                    unix_sock = nullptr;
                }
//                unix_sock = new QLocalSocket();
//                unix_sock->connectToServer(QString(server.data()), QLocalSocket::ReadOnly);
                break;

                //        case SP_ZMG_SUB:
                //            sprintf(addr_to_connect, "tcp://%s:%u", server.data(), port);
                //            z_sub_sock = zsocket_new(keep.czctx(), ZMQ_SUB);
                //            int st = zsocket_connect(z_sub_sock, "%s", addr_to_connect);
                //            if (-1 == st)
                //            {
                //                zsocket_destroy(keep.czctx(), z_sub_sock);
                //                z_sub_sock = 0;
                //                has_error = true;
                //            }
                //            else
                //                zmq_setsockopt (z_sub_sock, ZMQ_SUBSCRIBE, "", 0);

                //            break;
            default:
                return false;
                break;
            }
        }
        catch (Poco::Exception& exc)
        {
            std::cerr << "GenericStreamReader: " << exc.displayText() << std::endl;
            has_error = true;
        }


        if (has_error)
        {
            //emit error:
            daddy->propagate_error(make_mbytearray(__PRETTY_FUNCTION__));
            return false;
        }

        return true;
    }

    u_int64_t bytes_available()
    {
        int avb = 0;
        switch (sock_type) {
        case SP_UDP:
            avb = udp_sock->available();
            break;
        case SP_TCP:
            avb = tcp_sock->available();
            break;

        case SP_LOCAL:
//            avb = unix_sock->bytesAvailable();
            break;

//        case SP_ZMG_SUB:
//            break;

        default:
            break;
        }
        return avb;
    }

    u_int64_t read(void* data, size_t max_len)
    {
        u_int64_t ret = 0;
        try {
        switch (sock_type) {
        case SP_UDP:
            ret = udp_sock->receiveFrom(data, max_len, recv_addr, 0);
            break;
        case SP_TCP:
            ret = tcp_sock->receiveBytes(data, max_len);
            break;

        case SP_LOCAL:
//            ret = unix_sock->read((char*)data, max_len);
            break;

//        case SP_ZMG_SUB:
//            break;

        default:
            break;
        }
        } catch(Poco::Exception& pex)
        {
            std::cerr << "GenericStreamReader:" << pex.displayText() << std::endl;
        }
        return ret;
    }

    SHP(MByteArray) read()
    {
        std::lock_guard<std::mutex> lk(mut);
//        bool ct_do_start = false;
//        if (nullptr != chan_timer && chan_timer->isActive())
//        {
//            ct_do_start = true;
//            chan_timer->stop();
//        }

        u_int64_t avble = bytes_available();
        SHP(MByteArray) ba = make_mbytearray();
        if (avble > 0)
        {
           ba->resize(avble, 0x00);
           read(ba->unsafe(), avble);
        }
//        if (ct_do_start)
//        {
//            chan_timer->start();
//        }
        return ba;
    }

    std::shared_ptr<Poco::Net::StreamSocket> tcp_sock;
    std::shared_ptr<Poco::Net::DatagramSocket> udp_sock;
//    QLocalSocket* unix_sock;
//    //--- ZMQ SUB ---
//    zmq_ctx_keeper keep;
//    void* z_sub_sock;
//    char addr_to_connect[80];
//    //---------------

    Poco::Net::SocketAddress addr;
    Poco::Net::SocketAddress recv_addr;
    SHP(MByteArray) chunk;


    Chan<SHP(MByteArray)> datachan;
//    SHP(QTimer) chan_timer;


    GenericStreamReader* daddy;
    SP_TYPE sock_type;
    /** Server or unix socket name descripter*/
    std::string server;
    u_int16_t port;

    std::mutex mut;
};



GenericStreamReader::GenericStreamReader() : pv(std::make_shared<GSRPriv>(this))
{
    sidedata = 0;
}

GenericStreamReader::GenericStreamReader(const Json::Value& config)
    : GenericStreamReader()
{
    sidedata = 0;
    configure(config);
}

GenericStreamReader::~GenericStreamReader()
{

}

u_int64_t GenericStreamReader::bytes_available() const
{
   return pv->bytes_available();
}

SHP(MByteArray) GenericStreamReader::read() const
{
    return pv->read();
}

u_int64_t GenericStreamReader::read(void* dest, size_t max_len) const
{
    return pv->read(dest, max_len);
}

bool GenericStreamReader::is_configured() const
{
    return pv->sock_type != GSRPriv::SP_NONE;
}

std::mutex& GenericStreamReader::mutex() const
{
    return pv->mut;
}

bool GenericStreamReader::configure(const Json::Value& config)
{
    std::lock_guard<std::mutex> lk(pv->mut);
    bool ok = pv->jparse(config);
    if (!ok)
    {
        propagate_error(make_mbytearray("Bad GenericStreamReader construction."));
    }
    return ok;
}

void GenericStreamReader::propagate_error(std::shared_ptr<MByteArray> msg)
{
   if (0 != sig_error) sig_error(msg);
}


//Chan<SHP(MByteArray)> GenericStreamReader::start_channel(int period_msec)
//{
//    pv->datachan.reopen();
//    pv->chan_timer = ObjectsWatchdog::instance()->make_timer(true, period_msec);
//    connect(pv->chan_timer.get(), &QTimer::timeout,
//            this, &GenericStreamReader::read_if_available,
//            Qt::QueuedConnection);
//}

//void GenericStreamReader::stop_channel()
//{
//    OW_RECYCLE(pv->chan_timer);
//    std::move(pv->chan_timer);
//    pv->datachan.close();
//}

void GenericStreamReader::read_if_available()
{
    auto mb = pv->read();
    if (!mb->empty())
    {
        pv->datachan << mb;
    }
}

Chan<SHP(MByteArray)> GenericStreamReader::get_channel() const
{
   return pv->datachan;
}

}//namespace ZMBCommon
