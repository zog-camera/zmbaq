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


#ifndef ZMQ_CTX_KEEPER_H
#define ZMQ_CTX_KEEPER_H
#include <memory>
#include "czmq.h"
#include "zctx.h"
#include "zmbaq_common.h"
#include <mutex>

class KeepPV;

/** Used to mark messages.*/
struct z_identity_t
{
    z_identity_t() { generate(); }
    z_identity_t(const char* ident, int len = -1);

    //prepend or append zframe_t* to a message
    void make_frame(zmsg_t* msg, bool prepend = true) const;

    void generate();

    bool read(zmsg_t* identity_msg);

    bool empty() const;
    size_t size() const;
    size_t operator()() const;
    void invert();

    void zerofill();

    char identity[ZMBAQ_ZMQ_IDENTITY_LEN];
};

bool operator<(const z_identity_t& a, const z_identity_t& b);
bool operator==(const z_identity_t& a, const z_identity_t& b);
bool operator!=(const z_identity_t& a, const z_identity_t& b);

/** Holds the CZMQ message, frees it in destructor.
 * No locking, not threadsafe.
*/
class ZIdentedMessage
{
public:

    struct ZIMPv;

    /** Initiated with shared lock_guard for safe inter-threaded msg passing.*/
    class locked_msg
    {
    public:
        locked_msg();
        ~locked_msg();

        void release();

        zmsg_t* msg;

        std::shared_ptr<ZIMPv> pv;
        std::shared_ptr<std::lock_guard<std::mutex>> lk;
    };


    ZIdentedMessage();
    ZIdentedMessage(const z_identity_t& id, zmsg_t* a_msg);
    ZIdentedMessage(const z_identity_t& id, const Json::Value* json);
    virtual ~ZIdentedMessage();

    //delete the message:
    void clear();

    const z_identity_t& id();
    ZIdentedMessage::locked_msg get_locked_msg();
    zmsg_t* msg() const;

private:
    z_identity_t m_id;
    std::shared_ptr<ZIMPv> pv;

};
//--------------------------
class zmq_ctx_keeper
{
public:
    zmq_ctx_keeper();
    ~zmq_ctx_keeper();

    zctx_t* czctx();

private:
    std::shared_ptr<KeepPV> pv;
};

#endif // ZMQ_CTX_KEEPER_H
