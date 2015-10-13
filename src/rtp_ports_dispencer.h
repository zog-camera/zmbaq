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

#include "zmbaq_common.h"
#include <cstdint>
#include <string>
#include <deque>

class RTPPortsDispencer;
class RTPPortsDispencerPV;

//---------------------------------------------------
/** RTP ports pair (even, odd).
 *  Has implemented reference counting
 *  and call destructor only when last copy destroyed.
*/
class RTPPortPair
{
public:
    RTPPortPair();

    RTPPortPair(SHP(RTPPortsDispencerPV) dispencer,
                const uint16_t& a_even, const uint16_t& a_odd);
    virtual ~RTPPortPair();

    //release the port number, it will be returned to dispencer
    void reset();

    uint16_t even;
    uint16_t odd;

    //A tag may be set to show what this ports are used for
    //the tag has this 8-bytes limitation for optimal comparison operations
    //(re-interpreted as 64-bit interger)
    char tag[8];

    //may be invalid in rare case(when no more ports available):w
    bool valid() const {return 0 < (even & odd );}

    //get port dispencer's IP field
    ZConstString dispensers_ip() const;

    SHP(RTPPortsDispencerPV) dsp;
};
//---------------------------------------------------


class EvensArray;
typedef std::shared_ptr<EvensArray> EvensArrayPtr;

/** Ports pair dispencer.
 The RTPPortPair will release it's port when last copy
 of the RTPPortPair object is destroyed.
*/
class RTPPortsDispencer
{
public:
    //constructor with range:
    RTPPortsDispencer(uint16_t rlow = 6000, uint16_t rhi = 8000);
    virtual ~RTPPortsDispencer();

    /** The RTP pair may be optionally tagged. Only first 8 bytes are used.*/
    RTPPortPair obtain(ZConstString use_tag = ZConstString(0, (size_t)0));

    //set current IP field
    void set_destination_ip(const std::string& addr);

    //get current IP field
    ZConstString dest_ip() const;

    //register already obtained port(elsewhere)
    void bind(const RTPPortPair& item);

    void release(const RTPPortPair& item);

    //how much ports pairs available:
    uint16_t available() const;

    /** Get tagged ports.
     *  @param tag: must be 8bytes long.*/
    EvensArrayPtr get_tagged_ports(const ZConstString& tag) const;

private:
    SHP(RTPPortsDispencerPV) pv;

};
//---------------------------------------------------
#endif // RTPPORTSDISPENCER_H
