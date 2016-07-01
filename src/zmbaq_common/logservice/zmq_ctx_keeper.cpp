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


#include "zmq_ctx_keeper.h"
#include <mutex>
#include "jsoncpp/json/writer.h"

static std::shared_ptr<KeepPV> g_global_keep;
static std::mutex g_zkeep_mutex;




z_identity_t::z_identity_t(const char* ident, int len)
{
    memcpy(identity, ident, len > 0? ZMB::min<int>(len, (int)ZMBAQ_ZMQ_IDENTITY_LEN) : (int)ZMBAQ_ZMQ_IDENTITY_LEN);
}

void z_identity_t::zerofill()
{
    memset(identity, 0x00, ZMBAQ_ZMQ_IDENTITY_LEN);
}

size_t z_identity_t::size() const
{
    return ZMBAQ_ZMQ_IDENTITY_LEN;
}

bool z_identity_t::read(zmsg_t* identity_msg)
{
    zframe_t* identity = zmsg_first (identity_msg);
    if (nullptr == identity || zframe_size(identity) > ZMBAQ_ZMQ_IDENTITY_LEN)
        return false;
    return ((void*)identity) == memcpy(identity, zframe_data(identity), zframe_size(identity));
}

bool z_identity_t::empty() const
{
    register u_int64_t sum = 0u;
    register u_int64_t c = 0u;
    for (unsigned cnt = 0u; cnt < size() / sizeof(u_int64_t); ++cnt, c = 0u)
    {
       memcpy(&c, identity + cnt * sizeof(u_int64_t), sizeof(u_int64_t));
       sum += c;
    }
    return 0u == sum;
}

void z_identity_t::make_frame(zmsg_t* msg, bool prepend) const
{
   int res = 0;
   if (prepend)
   {
       res  = zmsg_pushmem(msg, (void*)identity, sizeof(identity));
   }
   else
   {
       res  = zmsg_addmem(msg, (void*)identity, sizeof(identity));
   }
   assert(0 == res);
}

void z_identity_t::generate()
{
    zerofill();
    snprintf (identity, 9,"%04X-%04X", rand() % 0x10000, rand() % 0x10000 );
}

size_t z_identity_t::operator()() const
{
    return ZMB::fletcher32((const u_int16_t*)identity, sizeof(identity));
}

void z_identity_t::invert()
{
    for (unsigned i = 0; i < sizeof(identity); ++i)
        identity[i] = 0xff - identity[i];
    identity[ZMBAQ_ZMQ_IDENTITY_LEN - 1] = 0x00;
}

bool operator<(const z_identity_t& a, const z_identity_t& b)
{
    return a() < b();
}

bool operator==(const z_identity_t& a, const z_identity_t& b)
{
    return 0 == memcmp(a.identity, b.identity, a.size());
}

bool operator!=(const z_identity_t& a, const z_identity_t& b)
{
    return !(a == b);
}

struct ZIdentedMessage::ZIMPv
{
    ZIMPv ()
    {
        msg = 0;
    }
    ~ZIMPv()
    {
        zmsg_destroy(&msg);
        msg = nullptr;
    }

    zmsg_t* msg;
    std::mutex mut;
};

ZIdentedMessage::locked_msg::locked_msg()
{
    msg = nullptr;
}

ZIdentedMessage::locked_msg::~locked_msg()
{
    release();
}

void ZIdentedMessage::locked_msg::release()
{
    msg = nullptr;
    lk.reset();
}


ZIdentedMessage::ZIdentedMessage()
{
    m_id.zerofill();
}

ZIdentedMessage::ZIdentedMessage(const z_identity_t& id, zmsg_t* a_msg)
 : pv(std::make_shared<ZIMPv>()), m_id(id)
{
    pv->msg = a_msg;
}

ZIdentedMessage::ZIdentedMessage(const z_identity_t& id, const Json::Value* json)
 : pv(std::make_shared<ZIMPv>()), m_id(id)
{
   pv->msg = zmsg_new();
   Json::FastWriter wr;
   auto str = wr.write(*json);
   id.make_frame(pv->msg);
   int res = zmsg_addmem(pv->msg, str.data(), str.size());
   assert(0 == res);
}

ZIdentedMessage::~ZIdentedMessage()
{

}

void ZIdentedMessage::clear()
{
    pv.reset();
}

const z_identity_t& ZIdentedMessage::id()
{
    return m_id;
}

ZIdentedMessage::locked_msg ZIdentedMessage::get_locked_msg()
{
    ZIdentedMessage::locked_msg lk_msg;
    lk_msg.lk = std::make_shared<std::lock_guard<std::mutex>> (pv->mut);
    lk_msg.msg = pv->msg;
    return lk_msg;
}

zmsg_t* ZIdentedMessage::msg() const
{

}
//----------------------------------------------
class KeepPV
{
public:
    KeepPV()
    {
        z = zctx_new();
    }
    ~KeepPV()
    {
        zctx_destroy(&z);
    }
    zctx_t* z;

};

zmq_ctx_keeper::zmq_ctx_keeper()
{
    std::lock_guard<std::mutex> lk(g_zkeep_mutex);
    if (g_global_keep == nullptr)
    {
       g_global_keep = std::make_shared<KeepPV>();
    }
    pv = g_global_keep;
}

zmq_ctx_keeper::~zmq_ctx_keeper()
{

}

//void* zmq_ctx_keeper::zmq_ctx()
//{
//    QMutexLocker lk(&g_zkeep_mutex);
//    return (void*)pv->z;
//}

zctx_t* zmq_ctx_keeper::czctx()
{
    std::lock_guard<std::mutex> lk(g_zkeep_mutex);
    return pv->z;
}
