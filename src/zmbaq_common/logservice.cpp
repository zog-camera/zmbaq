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


#include "logservice.impl.h"

#include "genericstreamwriter.h"
#include <Poco/DateTime.h>
#include <cassert>
#include "char_allocator_singleton.h"
#include "jsoncpp/json/writer.h"
#include "zmq_ctx_keeper.h"
#include <iostream>


//------------------------------------------------------------------
namespace logservice {

LogLevel llevel()
{
    instance();
}

LogService* instance()
{
    return LogService::instance();
}

LogService* instance(const Json::Value *dest_config)
{
     return LogService::instance(dest_config);
}

void linit(const Json::Value *dest_config, LogLevel lvl)
{
    auto LS = LogService::instance(dest_config);
    LS->start(lvl);
}

void lwrite(const ZConstString& obj_name, const ZConstString& msg, LogLevel lvl)
{
    auto LS = LogService::instance();
    LS->p_write(obj_name, msg, lvl,  Poco::DateTime());
}

void lwrite(const ZConstString& obj_name, const ZMBCommon::MByteArray& str, LogLevel lvl)
{
    auto LS = LogService::instance();
    LS->p_write(obj_name, ZConstString(str.data(), str.size()), lvl,  Poco::DateTime());
}

void lwrite(const ZConstString& obj_name, const ZMBCommon::MByteArrayList& msg, LogLevel lvl)
{
    auto LS = LogService::instance();
    SHP(ZMBCommon::MByteArray) mb = make_mbytearray();

    for (auto it = msg.begin(); it != msg.end(); ++it)
    {
        *mb += (*it);
    }
    LS->p_write(obj_name, ZConstString(mb->data(), mb->size()), lvl,  Poco::DateTime());
}

void lwrite(const ZConstString& obj_name, const std::string& str, LogLevel lvl)
{
    auto LS = LogService::instance();
    LS->p_write(obj_name, ZConstString(str.c_str(), str.size()), lvl,  Poco::DateTime());
}

void lwrite(const ZConstString& obj_name, SHP(ZMBCommon::MByteArray) str, LogLevel lvl)
{
    auto LS = LogService::instance();
    LS->p_write(obj_name, ZConstString(str->data(), str->size()), lvl,  Poco::DateTime());
}

void lwrite(const ZConstString& obj_name, const Json::Value& obj, LogLevel lvl)
{
    auto LS = LogService::instance();
    LS->p_write(obj_name, obj, lvl,  Poco::DateTime());
}

Json::Value ldefaults()
{
    Json::Value log_set;
    auto lk = ZMB::Settings::instance()->get_locker();
    log_set[ZMBCommon::GSW_KW_SOCKET.begin()] = *ZMB::Settings::instance()->s_get_jvalue(ZMB::ZMKW_LOGPROTO, lk);
    log_set[ZMBCommon::GSW_KW_SERVER.begin()] = *ZMB::Settings::instance()->s_get_jvalue(ZMB::ZMKW_LOGSERVER, lk);
    log_set[ZMBCommon::GSW_KW_PORT.begin()] = *ZMB::Settings::instance()->s_get_jvalue(ZMB::ZMKW_LOGPORT, lk);
    return log_set;
}

void ldestroy()
{
    static std::mutex dest_mutex;
    std::lock_guard<std::mutex> lk1(dest_mutex);
    auto lptr = LogService::instance();

    std::lock_guard<std::mutex> lk2(LogService::m_mutex);
    delete lptr;
    LogService::m_instance.store(0);
}

}
//------------------------------------------------------------------
using namespace logservice;
//------------------------------------------------------------------



std::atomic<LogService*> LogService::m_instance;
std::mutex LogService::m_mutex;

LogService* LogService::instance()
{
    LogService* tmp = m_instance.load();
    if (tmp == nullptr) {
        std::lock_guard<std::mutex> lock(m_mutex);
        tmp = m_instance.load();

        if (tmp == nullptr) {
            //call constructor on existing memory:
            tmp = new LogService();
            m_instance.store(tmp);
        }
    }
    return tmp;
}

LogService* LogService::instance(const Json::Value* dest_config)
{
    LogService* tmp = m_instance.load();
    if (tmp == nullptr) {
        std::lock_guard<std::mutex> lock(m_mutex);
        tmp = m_instance.load();

        if (tmp == nullptr) {
            //call constructor on existing memory:
            tmp = new LogService(*dest_config);
            m_instance.store(tmp);
        }
    }
    else
    {
        tmp->configure(*dest_config);
    }
    return tmp;
}



LogService::LogService(Json::Value config) : d_lvl(LogLevel::DEBUG)
{
    using_separate_thread = false;
    anything_goes = std::make_shared<ZMBCommon::GenericStreamWriter>();

    //default settings:
    configure(config);
}

LogService::~LogService()
{
    anything_goes->close();
    anything_goes.reset();
}

LogLevel LogService::level() const
{
   return d_lvl;
}


void LogService::configure(const Json::Value& dest_config)
{
    anything_goes->configure(dest_config);
}

void LogService::start(LogLevel lvl)
{
    d_lvl = lvl;
}

bool LogService::is_threaded() const
{
//    return d_conn_type == Qt::QueuedConnection;
    return false;
}

void create_log_obj(Json::Value& jo,
                    const char* obj_name,
                    LogLevel lvl,
                    Poco::DateTime dt = Poco::DateTime())
{
    int l = (int)lvl;
    assert(l < 4 && l >= 0);
    jo["object"] = (obj_name);
    jo["severity"] = (severity_strs[severity_strs_align * l]);
    char tmp[16]; memset(tmp, 0x00, sizeof(tmp));
    snprintf(tmp, 11,"%02d.%02d.%04d", dt.day(), dt.month(), dt.year());
    jo["date"] = tmp;
    snprintf(tmp, 6,"%02d:%02d", dt.hour(), dt.minute());
    jo["time"] = tmp;
    snprintf(tmp, 7,"%02d.%03d", dt.second(), dt.millisecond());
    jo["timesec"] = tmp;
    jo["timezone"] = "UTC+0";
}


void LogService::p_write(const ZConstString& obj_name, const ZConstString& msg,
                         LogLevel lvl, Poco::DateTime dt)
{
    std::lock_guard<std::mutex> lk(m_mutex);
    if (!anything_goes->is_configured())
    {
        std::cerr << "Logger ERROR. Not configured!\n";
        return;
    }
    if (lvl >= d_lvl)
    {
        Json::FastWriter d_wr;
        std::string d_buf;
        Json::Value d_jroot;


        bool on_heap_obj = obj_name.size() > 4096;
        char* obj = (char*) (on_heap_obj?
                                 malloc(obj_name.size() + 1) : alloca(obj_name.size() + 1));
        memset(obj, 0x00, obj_name.size() + 1);
        memcpy(obj, obj_name.begin(), obj_name.size());

        create_log_obj(d_jroot, obj, lvl, dt);

        d_buf = d_wr.write(d_jroot);
        if (!d_buf.empty())
        {
            zmsg_t* mz = zmsg_new();
            zframe_t* f1 = zframe_new(d_buf.data(), d_buf.size());
            zframe_t* f2 = zframe_new(msg.begin(), msg.size());
            zmsg_add(mz, f1);
            zmsg_add(mz, f2);
            anything_goes->send_zmsg(&mz);
            if (nullptr != mz)
                zmsg_destroy(&mz);
        }
        if (on_heap_obj)
            free(obj);
    }
}

void LogService::p_write(const ZConstString& obj_name, const Json::Value& obj, LogLevel lvl, Poco::DateTime dt )
{
    if (lvl >= d_lvl)
    {
        Json::FastWriter d_wr;
        std::string d_buf;
        d_buf = d_wr.write(obj);
        p_write(obj_name, ZConstString(d_buf.data(), d_buf.size()), lvl, dt);
    }
}
//----- these are called by other overloaded write() methods;
void LogService::write(ZMBCommon::MByteArrayPtr data, LogLevel lvl, Poco::DateTime dt)
{
    p_write(ZConstString("all", 3), ZConstString(data->data(), data->size()),
            lvl, dt);
}

void LogService::write(ZMBCommon::MByteArrayPtr obj_name, ZMBCommon::MByteArrayPtr data, LogLevel lvl, Poco::DateTime dt)
{
    p_write(ZConstString(obj_name->data(), obj_name->size()),
            ZConstString(data->data(), data->size()), lvl, dt);
}
//-----

void LogService::on_writer_error(std::shared_ptr<ZMBCommon::GenericStreamWriter> emitter,
                                 ZMBCommon::MByteArrayPtr msg)
{
//    sig_error(msg);
}

ZMBCommon::MByteArrayPtr LogService::make_ptr(const char* str) const
{
    return make_mbytearray(str);
}

ZMBCommon::MByteArrayPtr LogService::make_ptr(const ZMBCommon::MByteArray& str) const
{
    return make_mbytearray(str);
}



