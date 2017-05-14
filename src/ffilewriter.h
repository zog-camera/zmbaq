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
#include "packetspocket.h"
#include "fshelper.h"

namespace ZMB {

class FFileWriterErrorHandlerStub
{
 public:
  bool operator()(const std::string& msg)
  {
    std::cerr << "FFileWriter reports error: " << msg << "\n";
  }
};

/** Writer AV packets to file with container like .mp4 */
template<class FunctorOnError = FFileWriterErrorHandlerStub, class PImpl = FFileWriterPV>
class FFileWriter : public ZMBCommon::noncopyable
{
public:
  typedef std::unique_lock<std::mutex> Locker_t;

  
  FFileWriter();
  FFileWriter(const AVFormatContext *av_in_fmt_ctx, const std::string& dst);
  virtual ~FFileWriter();

    /** Synchronous and not thread-safe. Makes a copy of the packet.*/
  bool open(const AVFormatContext *av_in_fmt_ctx, const std::string& dst);
  void write(AVPacket *input_avpacket);
  void write(std::shared_ptr<av::Packet> pkt);
  void close();

  bool empty() const {return !is_open;}
  size_t file_size() const {return pb_file_size.load();}
  const std::string& path() const {return dest;}

  /** You can set/define functor */
  FunctorOnError onError;

  std::atomic_uint pb_file_size;
  /** just holds the reference. May be NULL if true == empty().*/
  ZMB::PacketsPocket pocket;//< frame buffering.
  ZMB::PacketsPocket::seq_key_t prevPktTimestamp;//< you modify it by hand

  void dumpPocketContent(ZMB::PacketsPocket::seq_key_t lastPacketTs);

  
  //things for locking data access:
  std::mutex& mutex() { return mu; };
  Locker_t&& locker()  { return std::move(std::unique_lock<std::mutex>(mu)); }

private:
  FFileWriterPV pv;
  std::mutex mu;
  std::string dest;
  bool is_open;
};

#include "ffilewriter_pv.inl.hpp"
 
}//ZMB

#endif //__FFILEWRITER__
