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



#ifndef LOGSERVICE_IMPL_H
#define LOGSERVICE_IMPL_H

#include "logservice.h"

#include <Poco/DateTime.h>
#include <memory>
#include "zmbaq_common.h"
#include "mbytearray.h"
#include <tbb/cache_aligned_allocator.h>
#include "jsoncpp/json/json.h"

class QThread;

namespace ZMBCommon {
    class GenericStreamWriter;
}

namespace logservice {

/** Must be created within std::shared_ptr<LogService> p(new LogService);
 *
  Writes messages to UDP socket in JSON:
  {"object":"log-item1",
 "severity":"debug",
  "date":"26.12.2014",
 "time":"13:49", "timesec":"45.001",
 "timezone":"UTC+2", "data": "message"}
*/
class LogService : public std::enable_shared_from_this<LogService>
{
public:
    explicit LogService(Json::Value config = ldefaults());
    ~LogService();

    LogLevel level() const;

    static LogService* instance();

    //construct with JSON config
    static LogService* instance(const Json::Value* dest_config);


    /** configure to work.
     * Supported protocols: "udp", "tcp", "unix".
     * Wrong configureation will cause sig_error() emission.
     *
     * @param config example:
     *  {"socket": "tcp", "server":"192.168.1.145", "port":59000},
     *  {"socket": "udp", "server":"192.168.1.145", "port":59000},
     *  {"socket": "unix", "descriptor": "/tmp/my_prog.socket"} */
    void configure(const Json::Value& dest_config);

    //whether moved to new thread:
    bool is_threaded() const;

    //synchronous write:
    void p_write(const ZConstString& obj_name, const ZConstString& msg,
               LogLevel lvl = DEBUG, Poco::DateTime dt = Poco::DateTime());

    void p_write(const ZConstString& obj_name, const Json::Value& obj,
                 LogLevel lvl = DEBUG, Poco::DateTime dt = Poco::DateTime());

    /** Start the service, only after this point get()
     *  will return non-NULL pointer.*/
    void start(LogLevel lvl = DEBUG);

//----- these are invoked by other async_write() methods;
    /** Write with obj_name = "all".*/
    void write(ZMBCommon::MByteArrayPtr data, LogLevel lvl = DEBUG, Poco::DateTime dt = Poco::DateTime());

    /** Write with specific obj_name.*/
    void write(ZMBCommon::MByteArrayPtr obj_name, ZMBCommon::MByteArrayPtr data,
               LogLevel lvl = DEBUG, Poco::DateTime dt = Poco::DateTime());


private:
    void on_writer_error(std::shared_ptr<ZMBCommon::GenericStreamWriter> emitter, ZMBCommon::MByteArrayPtr msg);


private:
    ZMBCommon::MByteArrayPtr make_ptr(const char* str) const;
    ZMBCommon::MByteArrayPtr make_ptr(const ZMBCommon::MByteArray& str) const;

private:
    static std::atomic<LogService*> m_instance;
    static std::mutex m_mutex;
    friend void logservice::ldestroy();

    LogLevel d_lvl;
    bool using_separate_thread;

    //main log hub:
    std::shared_ptr<ZMBCommon::GenericStreamWriter> anything_goes;
};

}//logservice

#endif // LOGSERVICE_IMPL_H
