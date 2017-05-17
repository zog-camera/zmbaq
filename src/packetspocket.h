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


#ifndef PACKETSPOCKET_H
#define PACKETSPOCKET_H
#include "minor/timingutils.h"
#include <deque>
#include <vector>
#include <memory>

extern "C"
{
#include "libavutil/rational.h"
}


#include "ffilewriter.h"

/*struct AVPacket;
namespace av
{
        class Packet;
        }*/

namespace ZMB {
  

class AFrameWriterDelegate
{
public:
  void operator()(AVPacket *packet) {(void)packet;}
};

/** Buffers packets that not older than @param_buffer_seconds. Allows to dump them to file/stream.*/
class PacketsPocket
{
public:

    /** Describes timestamped sequence of packets with a keyframe as the heading.*/

    typedef std::shared_ptr<av::Packet> AvPacketPtr;
    typedef std::pair<seq_key_t, AvPacketPtr> MarkedPacket;
    typedef std::vector<MarkedPacket> PacketsSequence;
    typedef std::pair<seq_key_t, PacketsSequence> TimedFrameSequencePair;
    //-----------------------------------------------------

    PacketsPocket();
    virtual ~PacketsPocket();

    void set(double param_buffer_seconds = 60,
             double param_minimum_movement_seconds = 2);

    /** Push packet, keep approx. stable size of the cached packets.*/
    seq_key_t push(AvPacketPtr pkt, bool do_delete_obsolete = true);


    /** Reset for each new file:*/
    TimingUtils::LapTimer laptime;
    
    /** Visit an object to dump all packets not present in file.
     * If pkt_stamp == 0, then gets all packets from range(0,buffering_seconds).
     * Else dumps all packets newer than the timestamp.
     * @param last_pkt_stamp -- upper limit of a timestamp of packets we're going to dump.
     * @param acceptorObject -- must have "<Any> accept(AvPacketPtr)" method;
     */

    template<class FrameAcceptor>
    void visit(seq_key_t& last_pkt_stamp, FrameAcceptor& acceptorObject);

protected:
    /** Frame sequences map. First frame in each sequence is a keyframe.*/
    typedef std::deque<TimedFrameSequencePair> multiseq_t;
    multiseq_t frames_sequences;

    /* Set if we need to make cache of frames for writing on
     *  MotionDetection algorithm's events.
     * 0 by default; */
    double buffering_seconds;
    double cur_buffered_time;

    double min_movement_seconds;


    int64_t prev_pts;
    AVRational t_base;

};

template<class FrameAcceptor>
void PacketsPocket::visit(seq_key_t& last_pkt_stamp, FrameAcceptor& acceptorObject)
{
  if (frames_sequences.empty())
    return;

  auto reverseIt = frames_sequences.rbegin();
  //find last keyframe that is older then given timestamp
  int64_t _pts = reverseIt->first.pts();
  for (;
       reverseIt != frames_sequences.rend() && _pts >= last_pkt_stamp.pts();
       ++reverseIt, _pts = reverseIt->first.pts())
  {/*span to find item*/
      
  }

  auto f_write_sequence = [&](const TimedFrameSequencePair& val, const seq_key_t& start_mark)
    {
      const PacketsSequence& deq(val.second);
      auto cursor = deq.begin();
      for (const MarkedPacket& packet : deq)
      {
        if (packet.first.pts() < start_mark.pts())
          continue;
        acceptorObject->accept(cursor->second);
        //I guess that there is always >= 1 element:
        last_pkt_stamp = packet.first;
      }
    };

  //write all packets newer than provided timestamp
  for (auto normalIterator = (reverseIt + 1).base();
       normalIterator != frames_sequences.end();
       ++normalIterator)
  {
    f_write_sequence(*normalIterator, last_pkt_stamp);
  }

}

}//namespace ZMB


#endif // PACKETSPOCKET_H
