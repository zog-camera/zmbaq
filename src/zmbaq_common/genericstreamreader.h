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


#ifndef GENERICSTREAMREADER_H
#define GENERICSTREAMREADER_H

#include <mutex>
#include "zmbaq_common.h"
#include "mbytearray.h"
#include "chan.h"

#include "genericstreamwriter.h"


namespace ZMBCommon {

class GSRPriv;

/** Look up some constants at GenericStreamWriter. */
class GenericStreamReader : public std::enable_shared_from_this<GenericStreamReader>
{
public:
    void* sidedata;
    std::function<void (SHP(MByteArray) /*msg*/)>       sig_error;
//    std::function<void(SHP(MByteArray)/*data bytes*/)>  sig_data_read;


    GenericStreamReader();

     /** Supported protocols: "udp", "tcp", "unix".
     * Wrong configureation will cause sig_error() emission.
     * Case using UDP protocol, it will bind the port and poll it.
     *
     * @param config example:
     *  {"socket": "tcp", "server":"192.168.1.145", "port":"9000"},
     *  {"socket": "udp", "server":"192.168.1.145", "port":"9000"},
     *  {"socket": "unix", "descriptor": "/tmp/my_prog.socket"}
     *  {"socket": "ZMG_SUB", "server":"192.168.1.145", "port":"9000"},
     *  Notice! The port is given as a string.
*/
    GenericStreamReader(const Json::Value& config);
    virtual ~GenericStreamReader();

    bool is_configured() const;

    //the channel to where data is pushed when start_channel() called.
    Chan<SHP(MByteArray)> get_channel() const;

    //bytes available
    u_int64_t bytes_available() const;

    u_int64_t read(void* dest, size_t max_len) const;

    SHP(MByteArray) read() const;

    //Get the mutex, it is locked while read() working.
    std::mutex& mutex() const;


    /** Accepts same configuration JSON as constructor.*/
    bool configure(const Json::Value& config);

    /** Start periodic socket polling.*/
//    Chan<SHP(MByteArray)> start_channel(int period_msec = 4);
//    void stop_channel();

    //checks if there is data and emits sig_data_read(ptr);
    void read_if_available();

    //emits sig_error(shared_from_this(), msg);
    void propagate_error(std::shared_ptr<MByteArray> msg);


private:
    SHP(GSRPriv) pv;
};

}

#endif // GENERICSTREAMREADER_H
