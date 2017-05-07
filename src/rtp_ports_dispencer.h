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


#ifndef RTPPORTSDISPENCER_H
#define RTPPORTSDISPENCER_H

#include "zmbaq_common/zmbaq_common.h"
#include <cstdint>
#include <cstring>
#include <string>
#include <deque>
#include <array>
#include "Poco/Channel.h"
#include "rtp_ports_dispencer_detail.hpp"

namespace ZMB {

typedef std::vector<uint16_t> EvensArray;
typedef std::shared_ptr<EvensArray> EvensArrayPtr;

class RTPPortsDispencer;

//---------------------------------------------------
/** RTP ports pair (even, odd).
 *  Has implemented reference counting
 *  and call destructor only when last copy destroyed.
*/
class RTPPortPair
{
public:
  //default ctor makes (0,0) which is invalid ports pair.
    RTPPortPair()
      : even(0), odd(0) { tag.fill(0); }
  
    RTPPortPair(uint16_t a_even, uint16_t a_odd)
      : even(a_even), odd(a_odd)   { }
  
    //may be invalid in rare case(when no more ports available):w
    bool valid() const {return 0 < (even & odd );}
    
    //release the port number, it will be returned to dispencer
    void reset() { *this = RTPPortPair();}
    
    //it will read 8 bytes exactly, last is guaranteed to be set to 0x00
    void readTag(const char* tagMSg)
    { * static_cast<u_int64_t*> (tag.data()) = * static_cast<u_int64_t*>(tagMsg);
      tag[--tag.size()] = 0x00; }

    uint16_t even;
    uint16_t odd;

    //A tag may be set to show what this ports are used for
    //the tag has this 8-bytes limitation for optimal comparison operations
    //(re-interpreted as 64-bit interger)
    std::array<char,sizeof(u_int64_t)> tag;

};
//---------------------------------------------------

 
/** Ports pair dispencer.
 The RTPPortPair will release it's port when last copy
 of the RTPPortPair object is destroyed.

 Thread safety: all method calls must be explicitly locked using a mutex from getMutex() method.
*/
class RTPPortsDispencer
{
public:
    //constructor with range:
    RTPPortsDispencer(uint16_t rlow = 6000, uint16_t rhi = 8000);
    virtual ~RTPPortsDispencer();

    /** The RTP pair may be optionally tagged. Only first 8 bytes are used.*/
    RTPPortPair obtain(ZConstString use_tag = ZConstString(0, (size_t)0));
    void release(const RTPPortPair& item);
    
    //set current IP field
    void set_destination_ip(const std::string& addr);

    //get current IP field
    std::string dest_ip() const;

    //how much ports pairs available:
    uint16_t available() const;

    /** Get tagged ports.
     *  @param tag: must be 8bytes long.*/
    EvensArrayPtr get_tagged_ports(const ZConstString& tag) const;

private:
    RTPPortsDispencerPV pv;

};

}//namespace ZMB

//---------------------------------------------------
#endif // RTPPORTSDISPENCER_H
