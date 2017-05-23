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
#include "mimage.h"

namespace ZMB {


//-----------------------------------------------------------------
PacketsPocket::PacketsPocket()
{
    prev_pts = 0;
    buffering_seconds = cur_buffered_time = min_movement_seconds = 0.0;
}

PacketsPocket::~PacketsPocket()
{

}

void PacketsPocket::set(double param_buffer_seconds, double param_minimum_movement_seconds)
{
   buffering_seconds = param_buffer_seconds;
   min_movement_seconds = param_minimum_movement_seconds;
}


ZMB::seq_key_t PacketsPocket::push(std::shared_ptr<av::Packet> pkt, bool do_delete_obsolete)
{
    t_base = pkt->timeBase().getValue();
    auto key = seq_key_t(laptime.elapsed(), pkt->pts().timestamp());

    if (pkt->isKeyPacket())
    {//is a keyframe, create a new packets sequence
        PacketsSequence deq;
        deq.emplace_back(std::move(MarkedPacket(key, pkt)));
        frames_sequences.push_back(TimedFrameSequencePair(key, deq));
    }
    else
    {//is a packet related to previous keyframe (if any)
        auto it = frames_sequences.rbegin();
        if (it != frames_sequences.rend())
        {
          //get the PacketsList:
          (*it).second.emplace_back(std::move(MarkedPacket(key, pkt)));
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


//-----------------------------------------------------------------


}//namespace ZMB
