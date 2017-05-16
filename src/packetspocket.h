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
    typedef std::pair<seq_key_t, AvPacketPtr> marked_pkt_t;
    typedef std::vector<marked_pkt_t> packet_seq_base_t;

    /** Has a keyframe as first element, other packets are related to that keyframe.*/
    class PacketsSequence : public packet_seq_base_t
    {
    public:
        using packet_seq_base_t::packet_seq_base_t;
        PacketsSequence& operator += (const marked_pkt_t& pkt);

    };

    typedef std::pair<seq_key_t, PacketsSequence> seq_item_t;
    //-----------------------------------------------------

    PacketsPocket();
    virtual ~PacketsPocket();

    void set(double param_buffer_seconds = 60,
             double param_minimum_movement_seconds = 2);

    /** Push packet, keep approx. stable size of the cached packets.*/
    seq_key_t push(AvPacketPtr pkt, bool do_delete_obsolete = true);

    /** Dump all packets not present in file.
     * If pkt_stamp == 0, then gets all packets from range(0,buffering_seconds).
     * Else dumps all packets newer than the timestamp. */
    void dump_packets(seq_key_t& last_pkt_stamp, ZMB::FFileWriter* pfile);

    /** Reset for each new file:*/
    TimingUtils::LapTimer laptime;

protected:
    /** Frame sequences map. First frame in each sequence is a keyframe.*/
    typedef std::deque<seq_item_t> multiseq_t;
    multiseq_t frames_sequences;

    /* Set if we need to make cache of frames for writing on
     *  MotionDetection algorithm's events.
     * 0 by default; */
    double buffering_seconds;
    double cur_buffered_time;

    double min_movement_seconds;


    int64_t prev_pts;
    char msg[256];
    AVRational t_base;

};

}//namespace ZMB


#endif // PACKETSPOCKET_H
