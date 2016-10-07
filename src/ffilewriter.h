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

struct AVFormatContext;

namespace ZMB {

/** Not pure virtual base. throws errors in runtime if pseudo-API
 * methods are called.*/
class FFileWriterBase : public ZMBCommon::noncopyable
{
public:
  typedef std::unique_lock<std::mutex> Locker_t;

  std::function<void(FFileWriterBase* fptr, const std::string& msg)>
  sig_error;

  /** */
  void raise_fatal_error();

  FFileWriterBase();
  virtual ~FFileWriterBase();

  virtual bool open(const AVFormatContext *av_in_fmt_ctx, const ZConstString& dst) = 0;

  virtual void close() = 0;

  /** child's method may increase pb_file_size. */
  virtual void write(AVPacket *input_avpacket) = 0;

  /** child's method may increase pb_file_size. */
  virtual void write(std::shared_ptr<av::Packet> pkt) = 0;


  bool empty() const {return !is_open;}
  size_t file_size() const {return pb_file_size.load();}
  const std::string& path() const {return dest;}

  std::mutex& mutex();
  std::unique_lock<std::mutex>&& locker();

  /** just holds the reference. May be NULL if true == empty().*/

  ZMB::PacketsPocket pocket;//< frame buffering.
  ZMB::PacketsPocket::seq_key_t prevPktTimestamp;//< you modify it by hand

  void dumpPocketContent(ZMB::PacketsPocket::seq_key_t lastPacketTs);

  std::atomic_uint pb_file_size;

protected:
  std::mutex mu;
  std::string dest;
  bool is_open;

  void dispatch_error(const std::string& msg)
  {
    if (nullptr != sig_error)
      sig_error(this, msg);
  }

};

//-------------------------------------------------------------------------

/** Writer AV packets to file with container like .mp4 */
class FFileWriter :  public ZMB::FFileWriterBase
{

public:
  FFileWriter();

  FFileWriter(const AVFormatContext *av_in_fmt_ctx, const ZConstString& dst);

  /** Warning! If inherited, you clean up this stuff self.*/
  virtual ~FFileWriter();

  /** Synchronous and not thread-safe. Makes a copy of the packet.*/
  virtual bool open(const AVFormatContext *av_in_fmt_ctx, const ZConstString& dst);
  void write(AVPacket *input_avpacket);
  void write(std::shared_ptr<av::Packet> pkt);
  void close();


private:
  struct PV;
  std::shared_ptr<PV> pv;
};

typedef std::shared_ptr<FFileWriter> FFileWriterPtr;
//------------------------------------------------------------


}//ZMB

#endif //__FFILEWRITER__
