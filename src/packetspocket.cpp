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


#include "packetspocket.h"
#include "external/avcpp/src/packet.h"
#include "ffilewriter.h"

namespace ZMB {


PacketsPocket::PacketsSequence& PacketsPocket::PacketsSequence::operator += (const marked_pkt_t& pkt)
{
    emplace_back(pkt);
    return *this;
}

//-----------------------------------------------------------------
PacketsPocket::PacketsPocket()
{
    prev_pts = 0;
    buffering_seconds = cur_buffered_time = min_movement_seconds = 0.0;

    memset(msg, 0x00, sizeof(msg));
}

PacketsPocket::~PacketsPocket()
{

}

void PacketsPocket::set(double param_buffer_seconds, double param_minimum_movement_seconds)
{
   buffering_seconds = param_buffer_seconds;
   min_movement_seconds = param_minimum_movement_seconds;
}


PacketsPocket::seq_key_t PacketsPocket::push(std::shared_ptr<av::Packet> pkt, bool do_delete_obsolete)
{
    t_base = pkt->timeBase().getValue();
    auto key = seq_key_t(laptime.elapsed(), pkt->pts().timestamp());
    marked_pkt_t mpkt (key, pkt);

    if (pkt->isKeyPacket())
    {//is a keyframe, create a new packets sequence
        PacketsSequence deq;
        deq += mpkt;
        frames_sequences.push_back(seq_item_t(key, deq));
    }
    else
    {//is a packet related to previous keyframe (if any)
        auto it = frames_sequences.rbegin();
        if (it != frames_sequences.rend())
        {
            //get the PacketsList:
            (*it).second += (mpkt);
        }
        //else we skip packets until first key-frame comes
    }
    prev_pts = pkt->pts().timestamp();

    if (!do_delete_obsolete || frames_sequences.size() <= 1)
        return key;

    auto it = frames_sequences.begin();
    ++it;//??
    seq_key_t mark = (*it).first;
    if (key.seconds() - mark.seconds() >= buffering_seconds)
    {
        frames_sequences.pop_front();
    }
    return key;
}

void PacketsPocket::dump_packets(seq_key_t& last_pkt_stamp, FFileWriterBase* pffilewriterbase)
{
    if (frames_sequences.empty())
        return;

    ZMB::FFileWriterBase* pfile = dynamic_cast<ZMB::FFileWriterBase*>(pffilewriterbase);
    /* Write the compressed frame to the media file. */
    assert(nullptr != pfile && false == pfile->empty());

    auto f_write_seq = [&](const multiseq_t::value_type& val, const seq_key_t& start_mark)
    {
        const PacketsSequence& deq(val.second);
        auto cursor = deq.begin();
        if (start_mark.pts() > 0)
        {
            for (auto it = deq.begin(); it != deq.end(); ++it)
            {
               if (it->first.pts() > start_mark.pts())
               {
                   cursor = it;
                   break;
               }
            }
        }
//        snprintf(msg, sizeof(msg),
//                 "file: %s frames sequence with %lu frames. Head timesec: %lf, pts: %lu\n",
//                pfile->path().data(), deq.size(), (*cursor).first, (*cursor).second);
//        LDEBUG(std::string("avpacket"), ZConstString(msg, strlen(msg)));

        for (; cursor != deq.end(); ++cursor)
        {
            pfile->write(cursor->second);
        }
        //I guess that there is always >= 1 element:
        last_pkt_stamp = cursor->first;
    };

    auto it = frames_sequences.begin();
    if (0 != last_pkt_stamp.pts())
    {
        it = frames_sequences.end();
        //find last keyframe that is older then given timestamp
        static const int64_t dull_pts = 0xFFFFFFFFFFFFFFFF;
        int64_t _pts = dull_pts;
        for (; it != frames_sequences.begin() && _pts > last_pkt_stamp.pts(); _pts = it->first.pts())
        {/*span to find item*/
           --it;
        }
    }
    //write all packets newer than provided timestamp
    for (; it != frames_sequences.end(); ++it)
    {
        f_write_seq(*it, last_pkt_stamp);
    }

}


//-----------------------------------------------------------------


}//namespace ZMB
