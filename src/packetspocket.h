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
#include "zmbaq_common.h"
#include "zmbaq_common/timingutils.h"
#include <deque>

extern "C"
{
#include "libavutil/rational.h"
}

struct AVPacket;
namespace av
{
        class Packet;
}

namespace ZMB {


/** Buffers packets that not older than @param_buffer_seconds. Allows to dump them to file/stream.*/
class PacketsPocket
{
public:
    /** A timestamp the double value is relative to the beginnning of the buffering.
     * pair(Time in seconds, pts in time_base) */
    class seq_key_t
    {
    public:
        seq_key_t() : d_seconds(0.0), d_pts_unit(0)  { }
        seq_key_t(const double& s, const int64_t& pts) : d_seconds(s), d_pts_unit(pts) {}
        const double& seconds() const {return d_seconds;}
        const int64_t& pts() const {return d_pts_unit;}
        double* p_seconds() {return &d_seconds;}
        int64_t* p_pts() {return &d_pts_unit;}

    private:
        double d_seconds;
        int64_t d_pts_unit;
    };

    /** Describes timestamped sequence of packets with a keyframe as the heading.*/

    typedef std::pair<seq_key_t, SHP(av::Packet)> marked_pkt_t;
    typedef std::vector<marked_pkt_t,tbb::cache_aligned_allocator<marked_pkt_t>> packet_seq_base_t;

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

    void set(double param_buffer_seconds = ZMBAQ_PACKETS_BUFFERING_SECONDS,
             double param_minimum_movement_seconds = ZMBAQ_MINIMAL_MOVEMENT_SECONDS);

    /** Push packet, keep approx. stable size of the cached packets.*/
    seq_key_t push(SHP(av::Packet) pkt, bool do_delete_obsolete = true);

    /** Dump all packets not present in file.
     * If pkt_stamp == 0, then gets all packets from range(0,buffering_seconds).
     * Else dumps all packets newer than the timestamp. */
    void dump_packets(seq_key_t& last_pkt_stamp, /*(FFileWriterBase*)*/ void* pffilewriterbase);

    /** Reset for each new file:*/
    TimingUtils::LapTimer laptime;

protected:
    /** Frame sequences map. First frame in each sequence is a keyframe.*/
    typedef std::deque<seq_item_t, tbb::cache_aligned_allocator<seq_item_t>> multiseq_t;
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
