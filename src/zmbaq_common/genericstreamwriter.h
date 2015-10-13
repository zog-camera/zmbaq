/*
A video surveillance software with support of H264 video sources.
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
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef GENERICSTREAMWRITER_H
#define GENERICSTREAMWRITER_H

#include "jsoncpp/json/json.h"
#include <mutex>
#include <memory>
#include "zmq_ctx_keeper.h"

class MByteArray;

namespace ZMBCommon {

class GSWPriv;

ZM_DEF_STATIC_CONST_STRING(GSW_UDP, "udp")
ZM_DEF_STATIC_CONST_STRING(GSW_TCP, "tcp")
ZM_DEF_STATIC_CONST_STRING(GSW_ZMQ, "ZMG_PUB")
ZM_DEF_STATIC_CONST_STRING(GSW_PRINTF, "printf")
ZM_DEF_STATIC_CONST_STRING(GSW_ZMQ_ADDR, "tcp://%s:%u")
ZM_DEF_STATIC_CONST_STRING(GSW_KW_SOCKET, "socket")
ZM_DEF_STATIC_CONST_STRING(GSW_KW_SERVER, "server")
ZM_DEF_STATIC_CONST_STRING(GSW_KW_PORT, "port")
ZM_DEF_STATIC_CONST_STRING(GSW_KW_DESCRIPTOR, "port")
ZM_DEF_STATIC_CONST_STRING(GSW_KW_AF_UNIX, "af_unix")
ZM_DEF_STATIC_CONST_STRING(GSW_KW_UNIX, "unix")
ZM_DEF_STATIC_CONST_STRING(GSW_KW_LOCAL, "local")
ZM_DEF_STATIC_CONST_STRING(GSW_VAL_LOCALHOST, "localhost")
ZM_DEF_STATIC_CONST_STRING(GSW_VAL_127001, "127.0.0.1")

bool gsw_jparse_server_port(std::string* server, u_int16_t* port, const Json::Value* config);

/** Wrapper around TCP, UDP, LOCAL sockets.
 *
 * Fot TCP and UDP protocols with URLs tcp:// or udp://,
 * this enity will
 * connect to remote host and will work as client.
 *
 * For AF_UNIX socket it'll work as server,
 * this will create, bind and listen the local socket
 * for a clients.
 *
*/
class GenericStreamWriter : public std::enable_shared_from_this<GenericStreamWriter>
{
public:
    void* sidedata;

    std::function<void(SHP(GenericStreamWriter) /*emitter*/,
                       std::shared_ptr<MByteArray> /*msg*/)>
                    sig_error;

    /** Only emitted by write() if 2nd "does_emit" argument was set 'true'.
     * @param data: original data pointer from input.
     * @param written: 0 if failed, equals to data->size() on success. */
    std::function<void(SHP(GenericStreamWriter)/* emitter*/,
                       std::shared_ptr<MByteArray> /*data*/, size_t written)>
                    sig_dispatch_status;

    GenericStreamWriter();

    /** Supported protocols: "udp", "tcp", "unix".
     * Wrong configureation will cause sig_error() emission.
     *
     * @param config example:
     *  {"socket": "tcp", "server":"192.168.1.145", "port":"9000"},
     *  {"socket": "udp", "server":"192.168.1.145", "port":"9000"},
     *  {"socket": "unix", "descriptor": "/tmp/my_prog.socket"},
     *  {"socket": "ZMG_PUB", "server":"192.168.1.145", "port":"9000"},
     *  {"socket": "std::cout"},
     *  Notice! The port is given as a string but may be a number as well.
*/
    GenericStreamWriter(const Json::Value& config);
    virtual ~GenericStreamWriter();

    /** Immidiate synchronous write.
     * @return n_bytes written.*/
    int64_t write(const char* data, size_t len);

    /** Notice! Only for ZMQ socket type! Assertion will raise otherwise.
     * @return ZMQ status code: 0 on success, -1 if failed. */
    int send_zmsg(zmsg_t** msg);

    /** Notice! Only for ZMQ socket type! Assertion will raise otherwise. */
    int send_zframe(zframe_t** frame_p, int flags = 0);

    /** Accepts same configuration JSON as constructor.*/
    bool configure(const Json::Value& config);

    bool is_configured() const;

    //Get the mutex, it is locked while write() working.
    std::mutex& mutex() const;

    // close socket
    void close();

    /** Dispatched write. It shall
     *  @param does_emit: whether sig_dispatch_status() should be emitted.*/
    int64_t write(std::shared_ptr<MByteArray> data, bool does_emit = false);

    //emits sig_error(shared_from_this(), msg);
    void propagate_error(std::shared_ptr<MByteArray> msg);

    //for internal use:
    void on_af_unix_connection();

private:
    std::shared_ptr<GSWPriv> pv;
};

typedef std::shared_ptr<ZMBCommon::GenericStreamWriter> GenericStreamWriterPtr;

}//namespace ZMBCommon

#endif // GENERICSTREAMWRITER_H
