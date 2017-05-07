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


#include "rtp_ports_dispencer.h"


void RTPPortPair::reset()
{
   even = 0;
   odd = 0;
   tag.fill(0);
}
//--------------------------------------------------------


RTPPortsDispencer::RTPPortsDispencer(uint16_t rlow, uint16_t rhi)
    : pv(RTPPortsDispencerPV(rlow, rhi))
{
    set_destination_ip("127.0.0.1");
}

RTPPortsDispencer::~RTPPortsDispencer()
{

}

void RTPPortsDispencer::set_destination_ip(const std::string& addr)
{
    pv.d_server_ip = addr;
}

std::string RTPPortsDispencer::dest_ip() const
{
    return pv.d_server_ip;
}

RTPPortPair RTPPortsDispencer::obtain(ZConstString use_tag)
{
    RTPPortPair p = pv.obtain();
    assert(nullptr != use_tag.begin());
    p.readTag(use_tag.begin());
    return p;
}

void RTPPortsDispencer::release(const RTPPortPair& item)
{
    pv.release(item);
}

uint16_t RTPPortsDispencer::available() const
{
    return pv.available();
}

EvensArrayPtr RTPPortsDispencer::get_tagged_ports(const ZConstString& tag) const
{
    return pv.m_tags.get_tagged_ports(tag);
}

}//namespace ZMB
