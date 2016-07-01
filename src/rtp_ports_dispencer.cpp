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
#include "cds_map.h"
#include <assert.h>
#include <tbb/cache_aligned_allocator.h>
#include "logservice.h"
#include <atomic>

//----------------------------------------------------------
class EvensArray : public std::deque<uint16_t, tbb::cache_aligned_allocator<uint16_t>>
{
public:

};
//----------------------------------------------------------
RTPPortPair::RTPPortPair(uint16_t a_even, uint16_t a_odd)
    : even(a_even), odd(a_odd)
{
}

RTPPortPair::RTPPortPair()
    : even(0), odd(0)
{
    tag.fill(0);
}
//----------------------------------------------------------

/** HashMap<uint16_t(even_port), uint16_t(odd_port)> */
typedef  std::map<uint16_t, uint16_t> PortsMap;
//------
/** HashMap<uint16_t(even_port), uint32_t(ref.counter)> */
typedef  std::map<uint16_t, u_int32_t> PortsMapRef;
//------

/** HashMap<uint64_t(char* tag), SHP(std::vector<uint16_t>) (even_port values list)> */
class TagsMap : public std::map<u_int64_t, EvensArrayPtr>
{
public:
    typedef std::map<u_int64_t, EvensArrayPtr> base_type;

    TagsMap() : base_type()
    {

    }

    //return tagged ports array (or empty array)
    EvensArrayPtr get_tagged_ports(const ZConstString& str_8bytes) const
    {
        u_int64_t num = 0;
        memcpy(&num, str_8bytes.begin(), ZMB::min<size_t>(sizeof(num), str_8bytes.size())  );
        EvensArrayPtr array;
        auto it = find(num);
        array = (end() == it)? (*it).second : std::make_shared<EvensArray>();
        return array;
    }

    //push tagged RTP even port
    void tag(const ZConstString& str_8bytes, uint16_t even_port)
    {
        u_int64_t num = 0;
        memcpy(&num, str_8bytes.begin(), ZMB::min<size_t>(sizeof(num), str_8bytes.size())  );
        EvensArrayPtr array;
        auto it = find(num);
        if (end() != it)
        {//case tag already exists:
            array = (*it).second;
        }
        else
        {//case need to create tag:
	    operator[](num) = std::make_shared<EvensArray>();
        }
        array->push_back(even_port);
    }
    //erase whole tag:
    void erase_tag(const ZConstString& str_8bytes)
    {
        u_int64_t num = 0;
        memcpy(&num, str_8bytes.begin(), ZMB::min<size_t>(sizeof(num), str_8bytes.size())  );

        auto iter = find(num);
        if (end() != iter)
        {
            erase(iter);
        }
    }

    //erase one tagged port:
    void remove_tagged_port(const ZConstString& str_8bytes, uint16_t port)
    {
        u_int64_t num = 0;
        memcpy(&num, str_8bytes.begin(), ZMB::min<size_t>(sizeof(num), str_8bytes.size())  );
        if (num > 0)
        {
            auto it = find(num);
            if (end() == it)
                return;

            auto arrayp = (*it).second;
            for (auto array_iter = arrayp->begin(); array_iter != arrayp->end(); ++array_iter)
            {
                if (port == *array_iter)
                {
                    arrayp->erase(array_iter);
                    break;
                }
            }
        }
    }
};

//------

/** Keeps lists of used RTP/RTCP ports.*/
class RTPPortsDispencerPV : public std::enable_shared_from_this<RTPPortsDispencerPV>
{
public:
    RTPPortsDispencerPV(uint16_t rlow, uint16_t rhi)
        : low(rlow), hi(rhi)
    {
        auto it = m_free.begin();
        bool ok = true;
        //insert (even, odd) pairs
        for (uint16_t c = rlow; c < rhi && ok; c += 2)
        {
            ok = it.prepend(c, c + 1);
        }
        assert(ok);
    }
    ~RTPPortsDispencerPV()
    {
    }

    RTPPortPair obtain(ZConstString tag = ZConstString(0,(size_t)0))
    {
        uint16_t even = 0, odd = 0;
        auto iter = m_free.begin();
        if (m_free.end() != iter)
        {
            even = (*iter).first;
            odd = (*iter).second;
            //same as this->incref(even);
	    m_map_ref[even] = 1u;
	    m_free.erase(iter);
        }
        else
        {
            //no free ports, some shit happend
            AERROR(ZCSTR("No free UDP ports left!"));
            assert(false);
        }
        if (nullptr != tag.begin())
        {
            m_tags.tag(tag, even);
        }
        return RTPPortPair(even, odd);
    }

    size_t available()
    {
        return m_free.size();
    }

    void incref(uint16_t even)
    {
        auto it = m_map_ref.find(even);
        if (m_map_ref.end() != it)
        {
            ++(*it);
        }
        else
        {
	    (*it) = 1u;
        }

    }
    //return refcount:
    int decref(uint16_t even)
    {
        auto it = m_map_ref.find(even);
        if (m_map_ref.end() != it)
        {
            --(*it);
            return it.value();
        }
        return 0;
    }

    void release(const RTPPortPair& item)
    {
        if (0 == decref(item.even))
        {
            m_tags.remove_tagged_port(item.tag, item.even);
            auto mg = m_free.mg();
            mg.second->emplace(item.even, item.odd);
        }
    }


    /** Defaults to 127.0.0.1. Must be changed when needed. */
    std::string d_server_ip;
    TagsMap m_tags;

private:
    PortsMap m_free;
    PortsMapRef m_map_ref;
    uint16_t low, hi;

};


void RTPPortPair::reset()
{
   even = 0;
   odd = 0;
   tag.fill(0);
}
//--------------------------------------------------------


RTPPortsDispencer::RTPPortsDispencer(uint16_t rlow, uint16_t rhi)
    : pv(std::make_shared<RTPPortsDispencerPV>(rlow, rhi))
{
    set_destination_ip("127.0.0.1");
}

RTPPortsDispencer::~RTPPortsDispencer()
{

}

void RTPPortsDispencer::set_destination_ip(const std::string& addr)
{
    pv->d_server_ip = addr;
}

std::string RTPPortsDispencer::dest_ip() const
{
    return pv->d_server_ip;
}

RTPPortPair RTPPortsDispencer::obtain(ZConstString use_tag)
{
    RTPPortPair p = pv->obtain();
    if (nullptr != use_tag.begin())
    {
        memcpy(p.tag, use_tag.begin(), ZMB::min<size_t>(sizeof(p.tag) - 1, use_tag.size()) );
    }
    return p;
}

void RTPPortsDispencer::release(const RTPPortPair& item)
{
    pv->release(item);
}

uint16_t RTPPortsDispencer::available() const
{
    return pv->available();
}

void RTPPortsDispencer::bind(const RTPPortPair& item)
{
    pv->incref(item.even);
}

EvensArrayPtr RTPPortsDispencer::get_tagged_ports(const ZConstString& tag) const
{
    return pv->m_tags.get_tagged_ports(tag);
}

