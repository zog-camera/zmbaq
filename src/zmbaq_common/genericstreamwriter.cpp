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


#include "genericstreamwriter.h"
#include <iostream>
#include <mutex>

#include "char_allocator_singleton.h"
#include "mbytearray.h"
#include "zmbaq_common.h"
#include "zmq_ctx_keeper.h"

#include <czmq.h>
#include "Poco/Net/DatagramSocket.h"
#include "Poco/Net/StreamSocket.h"

namespace ZMBCommon {

static int64_t gsw_udp_write(const std::shared_ptr<GSWPriv>& impl, const char* data, size_t len);
static int64_t gsw_tcp_write(const std::shared_ptr<GSWPriv>& impl, const char* data, size_t len);
static int64_t gsw_unix_write(const std::shared_ptr<GSWPriv>& impl, const char* data, size_t len);
static int64_t gsw_zmq_write(const std::shared_ptr<GSWPriv>& impl, const char* data, size_t len);
static int64_t gsw_stdio_write(const std::shared_ptr<GSWPriv>& impl, const char* data, size_t len);

class GSWZPublisherCtx : public ZMB::noncopyable
{
public:
    GSWZPublisherCtx(const MByteArray& server, u_int16_t port)
        : m_server(server), m_port(port)
    {
       publisher = zsocket_new(keep.czctx(), ZMQ_PUB);
       addr.resize(128, 0x00);

       snprintf(addr.unsafe(), server.size() + 13, "tcp://%s:%u", server.data(), port);
       int rc = zmq_bind(publisher, addr.data());
    }

    int s_send (void *socket, const char *string, size_t len) {
        zmq_msg_t message;
//        zmq_msg_init_size (&message, len);
//        memcpy (zmq_msg_data (&message), string, len);

        zmq_msg_init_data(&message, (void*)string, len, 0, 0);
        int size = zmq_msg_send (&message, socket, 0);
        zmq_msg_close (&message);
        return (size);
    }
    int send_zframe(zframe_t** frame_p, int flags = 0)
    {
        return zframe_send(frame_p, publisher, flags);
    }

    int write(const char* data, size_t len)
    {
        std::lock_guard<std::mutex> lk(mut);
        if (nullptr != data && nullptr != publisher)
        {
            return s_send(publisher, data, len);
        }
        return 0;
    }
    void close()
    {
        std::lock_guard<std::mutex> lk(mut);
        zsocket_destroy(keep.czctx(), publisher);
        publisher = nullptr;
    }
    const MByteArray& server() const {return m_server;}
    u_int16_t port() const {return m_port;}

    ~GSWZPublisherCtx()
    {
        close();
    }

    MByteArray m_server;
    u_int16_t m_port;
    std::mutex mut;
    MByteArray addr;
    zmq_ctx_keeper keep;
    void* publisher;

};

bool gsw_jparse_server_port(std::string* server, u_int16_t* port, const Json::Value* config)
{
    bool has_error = true;
    bool ok = true;
    const Json::Value* jpsrv = 0, *jpport = 0;
    if (ZMBCommon::GSW_KW_SERVER.is_key_of(config, &jpsrv))
    {
        *server = jpsrv->asCString();
        has_error = false;
    }
    if (ZMBCommon::GSW_KW_PORT.is_key_of(config, &jpport))
    {
        if (jpport->isString())
        {
            *port = ::atoi(jpport->asCString());
        }
        else
        {
            *port = jpport->asInt();
        }
        has_error = !ok;
    }
    return !has_error;
}

class GSWPriv
{
public:

    enum SP_TYPE {SP_NONE = 0x00, SP_UDP, SP_TCP, SP_LOCAL,
                 SP_UDP_SRV, SP_TCP_SRV, SP_ZMG_PUB, SP_PRINTF};

    GSWPriv(ZMBCommon::GenericStreamWriter* parent) : daddy(parent),
        sock_type(SP_NONE),
        server("localhost"),
        port(9123),
        pfn_write(nullptr)
    {
//        af_unix_client = nullptr;
        af_unix_inited = false;
        m_is_open = false;
        udp_sock = nullptr;
        tcp_sock = nullptr;
//        unix_sock_srv = nullptr;
    }
    ~GSWPriv()
    {
        close();
    }

    bool open_by_json(const Json::Value&config)
    {
        if (!config.isMember("socket"))
            return false;

        MByteArray pr;
        pr += config[ZMBCommon::GSW_KW_SOCKET.begin()].asCString();
        if (ZMBCommon::GSW_UDP == pr)
           sock_type = SP_UDP;
        else
        if (ZMBCommon::GSW_TCP == pr)
           sock_type = SP_TCP;
        else
        if (ZMBCommon::GSW_ZMQ == pr)
            sock_type = SP_ZMG_PUB;
        else
        if (ZMBCommon::GSW_PRINTF == pr)
            sock_type = SP_PRINTF;

        bool has_error = true;
        u_int16_t oldport = port;
        if (sock_type != SP_LOCAL && sock_type != SP_NONE)
        {
            has_error = !gsw_jparse_server_port(&server, &port, &config);
        }
        else if (ZMBCommon::GSW_KW_LOCAL == pr || ZMBCommon::GSW_KW_UNIX == pr || ZMBCommon::GSW_KW_UNIX == pr)
        {
            sock_type = SP_LOCAL;
            const Json::Value* jdesc = 0;
            if (ZMBCommon::GSW_KW_DESCRIPTOR.is_key_of(&config, &jdesc))
            {
               server = jdesc->asCString();
               has_error = false;
            }
        }
        else if (ZMBCommon::GSW_PRINTF == pr)
        {
            has_error = false;
            m_is_open = true;
            pfn_write = &gsw_stdio_write;
            return true;
        }

        if (has_error)
        {
            //emit error:
            daddy->propagate_error(std::make_shared<MByteArray>(__PRETTY_FUNCTION__));
        }
        if (server == ZMBCommon::GSW_VAL_LOCALHOST.begin())
        {
            server = ZMBCommon::GSW_VAL_127001.begin();
        }

        if (m_is_open && oldport == port && addr.host().toString() == server)
        {
            //called with same configuration.
            return true;
        }
        else
        {
            close();
        }

        addr = Poco::Net::SocketAddress(Poco::Net::IPAddress(server), port);
        switch (sock_type) {
        case SP_UDP:
            udp_sock = std::make_shared<Poco::Net::DatagramSocket>(Poco::Net::IPAddress::IPv4);
            udp_sock->setSendBufferSize(64 * 1024);
            pfn_write = &gsw_udp_write;
            break;
        case SP_TCP:
            tcp_sock = std::make_shared<Poco::Net::StreamSocket>();
            tcp_sock->setSendBufferSize(64 * 1024);
            tcp_sock->connect(addr);
            tcp_sock->shutdownReceive();
            pfn_write = &gsw_tcp_write;
            break;

//        case SP_LOCAL:
//            unix_sock_srv = new QLocalServer();
//            unix_sock_srv->listen(QString(server.data()));
//            pfn_write = &gsw_unix_write;
//            QObject::connect(unix_sock_srv, &QLocalServer::newConnection,
//                              daddy, &ZMBCommon::GenericStreamWriter::on_af_unix_connection, Qt::QueuedConnection);
//            break;

        case SP_ZMG_PUB:
            if (nullptr == zm || zm->server() != server.data() || zm->port() != port)
            {
                if (nullptr != zm.get())
                {
                    zm->close();
                }
                zm = std::make_shared<GSWZPublisherCtx>(MByteArray(server), port);
                pfn_write = &gsw_zmq_write;
            }
            break;
        case SP_PRINTF:
            pfn_write = &gsw_stdio_write;
            break;

        default:
            return false;
            break;
        }
        m_is_open = true;
        return true;

    }
    bool is_open() const
    {
       return m_is_open;
    }

    void close()
    {
        if (!m_is_open)
            return;

        std::lock_guard<std::mutex> lk(mut);
        pfn_write = nullptr;
        switch (sock_type) {
        case SP_UDP:
            udp_sock->close();
            std::move(udp_sock);
            break;
        case SP_TCP:
            tcp_sock->close();
            std::move(tcp_sock);
            break;

        case SP_LOCAL:
//            unix_sock_srv->deleteLater();
//            unix_sock_srv = nullptr;
            af_unix_inited = false;
            break;

        case SP_ZMG_PUB:
            zm->close();
            std::move(zm);
            break;
        default:
            break;
        }
        sock_type = SP_NONE;
        m_is_open = false;
        addr = Poco::Net::SocketAddress();
        server.clear();
    }

    bool m_is_open;
    std::mutex mut;
    int64_t (*pfn_write)(const std::shared_ptr<GSWPriv>& impl, const char* data, size_t len);

    SHP(Poco::Net::StreamSocket) tcp_sock;
    SHP(Poco::Net::DatagramSocket) udp_sock;
//    QLocalServer* unix_sock_srv;
//    QLocalSocket* af_unix_client;
    bool af_unix_inited;

    SHP(GSWZPublisherCtx) zm;

    ZMBCommon::GenericStreamWriter* daddy;
    SP_TYPE sock_type;
    /** Server or unix socket name descripter*/
    Poco::Net::SocketAddress addr;
    u_int16_t port;
    std::string server;
};

static int64_t gsw_udp_write(const std::shared_ptr<GSWPriv>& impl,
                             const char* data, size_t len)
{
    if (!impl->is_open())
        return 0u;
    int64_t wr = -1;
    try {
        wr = impl->udp_sock->sendTo(data, len, impl->addr);
    }
    catch (Poco::Exception& exc)
    {
        std::cerr << "GenericStreamWriter: " << exc.displayText() << std::endl;
    }
    return wr;
}

static int64_t gsw_tcp_write(const std::shared_ptr<GSWPriv>& impl,
                             const char* data, size_t len)
{
    if (!impl->is_open())
        return 0u;
    return impl->tcp_sock->sendBytes(data, len);
}

static int64_t gsw_unix_write(const std::shared_ptr<GSWPriv>& impl,
                              const char* data, size_t len)
{
    if (!impl->is_open())
        return 0u;

//    if (impl->af_unix_inited)
//    {
//        return impl->af_unix_client->write(data, len);
//    }
    return 0;
}

static int64_t gsw_zmq_write(const std::shared_ptr<GSWPriv>& impl,
                             const char* data, size_t len)
{
    if (!impl->is_open())
        return 0u;

    if (nullptr != impl->zm)
    {
        return impl->zm->write(data, len);
    }
    return 0;
}

#include <cstdio>

static int64_t gsw_stdio_write(const std::shared_ptr<GSWPriv>& impl, const char* data, size_t len)
{
   return ::fwrite(data, len, 1, ::stdout);
}

GenericStreamWriter::GenericStreamWriter() : pv(std::make_shared<GSWPriv>(this))
{
    sidedata = nullptr;
}

GenericStreamWriter::GenericStreamWriter(const Json::Value& config) : GenericStreamWriter()
{
    sidedata = nullptr;
    configure(config);
}

GenericStreamWriter::~GenericStreamWriter()
{
}

std::mutex& GenericStreamWriter::mutex() const
{
   return pv->mut;
}

int GenericStreamWriter::send_zmsg(zmsg_t** msg)
{
    if (GSWPriv::SP_ZMG_PUB != pv->sock_type)
    {
        zframe_t* f1 = zmsg_first(*msg);
        write((const char*)zframe_data(f1), zframe_size(f1));
        zframe_t* fnext = zmsg_next(*msg);
        for(; nullptr != fnext; fnext = zmsg_next(*msg))
        {
            write((const char*)zframe_data(fnext), zframe_size(fnext));
        }
        return 0;
    }

    std::lock_guard<std::mutex> lk(pv->mut);
    if (!pv->is_open())
    {
        return -1;
    }
    return zmsg_send(msg, pv->zm->publisher);
}

int GenericStreamWriter::send_zframe(zframe_t** frame_p, int flags)
{
    if (GSWPriv::SP_ZMG_PUB != pv->sock_type)
    {
        write((const char*)zframe_data(*frame_p), zframe_size(*frame_p));
        return 0;
    }

    std::lock_guard<std::mutex> lk(pv->mut);
    if (!pv->is_open())
    {
        return -1;
    }
    return pv->zm->send_zframe(frame_p, flags);
}

int64_t GenericStreamWriter::write(const char* data, size_t len)
{
    std::lock_guard<std::mutex> lk(pv->mut);
    if (!pv->is_open())
    {
        return -1;
    }
    return pv->pfn_write(pv, data, len);
}

int64_t GenericStreamWriter::write(std::shared_ptr<MByteArray> data, bool does_emit)
{
    if (nullptr != data)
    {
        int64_t written = write(data->data(), data->size());
        if (does_emit)
        {
            sig_dispatch_status(shared_from_this(), data, written);
        }
        return written;
    }
    return -1;
}

void GenericStreamWriter::propagate_error(std::shared_ptr<MByteArray> msg)
{
   sig_error(shared_from_this(), msg);
}

void GenericStreamWriter::on_af_unix_connection()
{
//   pv->af_unix_client = pv->unix_sock_srv->nextPendingConnection();
   pv->af_unix_inited = true;
}

bool GenericStreamWriter::is_configured() const
{
    return nullptr != pv && pv->is_open();
}

bool GenericStreamWriter::configure(const Json::Value& config)
{
    std::lock_guard<std::mutex> lk(pv->mut);
    bool ok = false;
    try {
        ok = pv->open_by_json(config);
    }
    catch (Poco::Exception& exc)
    {
        std::cerr << exc.displayText() << std::endl;
        ok = false;
    }

    assert(ok);
    if (!ok)
    {
        propagate_error(make_mbytearray("Bad GenericStreamWriter construction."));
    }
    return ok;
}

void GenericStreamWriter::close()
{
    if (pv->is_open())
    {
        try {
            pv->close();
        }
        catch (Poco::Exception& exc)
        {
            std::cerr << exc.displayText() << std::endl;
            assert(false);
        }
    }
}

} //namespace ZMBCommon
