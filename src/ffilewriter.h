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


#ifndef __FFILEWRITER__
#define __FFILEWRITER__

#include <memory>
#include <string>
#include <iostream>
#include <boost/noncopyable.hpp>
#include "packetspocket.h"
#include "fshelper.h"
extern "C"
{
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavutil/mathematics.h>
    #include <libavutil/timestamp.h>
}
#include "external/avcpp/src/packet.h"
#include <cassert>
#include <atomic>
#include <utility>
#include "minor/lockable.hpp"

namespace ZMB {

  /** A timestamp the double value is relative to the beginnning of the buffering.
   * pair(Time in seconds, pts in time_base) */
  class seq_key_t : public std::pair<double, int64_t>
  {
  public:
  seq_key_t() : d_seconds(0.0), d_pts_unit(0)  { }
  seq_key_t(const double& s, const int64_t& pts) : d_seconds(s), d_pts_unit(pts) {}
    
    double seconds() const {return first;}
    int64_t pts() const {return second;}

    double& seconds() {return first;}
    int64_t& pts() {return second;}
    

  private:
    double d_seconds;
    int64_t d_pts_unit;
  };
  static bool operator < (const seq_key_t& lhs, const seq_key_t& rhs)
  {
    return lhs.pts() < rhs.pts();
  }

  class FFileWriterErrorHandlerStub
  {
  public:
    bool operator()(const std::string& msg)  {
      std::cerr << "FFileWriter reports error: " << msg << "\n";
    }
  };

  template<class T>
    class FFileWriterPV;


/** Writer AV packets to file with container like .mp4 */
  template<class PImpl = FFileWriterPV<FFileWriterErrorHandlerStub> >
    class FFileWriter : public boost::noncopyable
    {
    public:
    typedef std::unique_lock<std::mutex> Locker_t;

  
    FFileWriter();
    FFileWriter(const AVFormatContext *av_in_fmt_ctx, const std::string& dst);
    virtual ~FFileWriter();

    //accept packets from visitors:
    void accept(std::shared_ptr<av::Packet> pkt)
    { write(pkt); }
    
    //accept packets from visitors:    
    void accept(AVPacket* pkt)
    { write(pkt); }

    /** Synchronous and not thread-safe. Makes a copy of the packet.*/
    bool open(const AVFormatContext *av_in_fmt_ctx, const std::string& dst);
    void write(AVPacket *input_avpacket);
    void write(std::shared_ptr<av::Packet> pkt);
    

    void close();

    bool empty() const {return !is_open;}
    size_t file_size() const {return pv.pb_file_size.load();}
    const std::string& path() const {return dest;}

    /** You can set/define functor */

    /** just holds the reference. May be NULL if true == empty().*/
    ZMB::seq_key_t prevPktTimestamp;//< you modify it by hand if you need it
  
    //things for locking data access:
    ZMB::LockableObject pbLockable;

    private:
    PImpl pv;
    std::string dest;
    bool is_open;
    };

  template<class PImpl>
    FFileWriter<PImpl>::FFileWriter()
  {
    is_open = false;
  }

  template<class PImpl>
    FFileWriter<PImpl>::FFileWriter(const AVFormatContext *av_in_fmt_ctx, const std::string& dst)
  {
    open(av_in_fmt_ctx, dst);
  }

  template<class PImpl>
    FFileWriter<PImpl>::~FFileWriter()
  {
    close();
  }

  template<class PImpl>
    bool FFileWriter<PImpl>::open(const AVFormatContext *av_in_fmt_ctx, const std::string& dst)
  {
    pv = PImpl(this, av_in_fmt_ctx, dst);
    is_open = pv.open(dst);
    return is_open;
  }

  template<class PImpl>
    void FFileWriter<PImpl>::write(std::shared_ptr<av::Packet> pkt)
  {
    pv.write(pkt);
  }

  template<class PImpl>
    void FFileWriter<PImpl>::close()
  {
    if (is_open)
      pv.close();
    is_open = false;
  }

}//ZMB

#include "ffilewriter_pv.inl.hpp"

#endif //__FFILEWRITER__
