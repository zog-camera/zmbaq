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
RTPPortPair::RTPPortPair(SHP(RTPPortsDispencerPV) dispencer,
                         const uint16_t& a_even, const uint16_t& a_odd)
    : even(a_even), odd(a_odd), dsp(dispencer)
{
    memset(tag, 0x00, sizeof(tag));
}

RTPPortPair::RTPPortPair()
    : even(0), odd(0)
{

}
//----------------------------------------------------------

/** HashMap<uint16_t(even_port), uint16_t(odd_port)> */
typedef  LCDS::MapHnd<uint16_t, uint16_t> PortsMap;
//------
/** HashMap<uint16_t(even_port), uint32_t(ref.counter)> */
typedef  LCDS::MapHnd<uint16_t, u_int32_t> PortsMapRef;
//------

/** HashMap<uint64_t(char* tag), SHP(std::vector<uint16_t>) (even_port values list)> */
class TagsMap : public LCDS::MapHnd<u_int64_t, EvensArrayPtr>
{
public:
    typedef LCDS::MapHnd<u_int64_t, EvensArrayPtr> base_type;

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
        array = (it.is_end())? it.value() : std::make_shared<EvensArray>();
        return array;
    }

    //push tagged RTP even port
    void tag(const ZConstString& str_8bytes, uint16_t even_port)
    {
        u_int64_t num = 0;
        memcpy(&num, str_8bytes.begin(), ZMB::min<size_t>(sizeof(num), str_8bytes.size())  );
        EvensArrayPtr array;
        auto it = find(num);
        if (!it.is_end())
        {//case tag already exists:
            array = it.value();;
        }
        else
        {//case need to create tag:
            emplace(it, num, std::make_shared<EvensArray>());
        }
        array->push_back(even_port);
    }
    //erase whole tag:
    void erase_tag(const ZConstString& str_8bytes)
    {
        auto m_guard = mg();
        u_int64_t num = 0;
        memcpy(&num, str_8bytes.begin(), ZMB::min<size_t>(sizeof(num), str_8bytes.size())  );

        auto iter = m_guard.second->map().find(num);
        if (iter != m_guard.second->map().end())
        {
            m_guard.second->map().erase(iter);
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
            if (it.is_end())
                return;

            auto arrayp = it.value();
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
        if (!iter.is_end())
        {
            even = iter.key();
            odd = iter.value();
            //same as this->incref(even);
            m_map_ref.emplace(even, (1u), m_map_ref.m());
            iter.erase();
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
        auto pair = RTPPortPair(shared_from_this(), even, odd);
        pair.dsp = shared_from_this();
        return pair;
    }

    size_t available()
    {
        return m_free.begin().map_size();
    }

    void incref(const uint16_t& even)
    {
        auto it = m_map_ref.find(even);
        if (!it.is_end())
        {
            ++it.value();
        }
        else
        {
            it.prepend(even, 1u);
        }

    }
    //return refcount:
    int decref(const uint16_t& even)
    {
        auto it = m_map_ref.find(even);
        if (!it.is_end())
        {
            --it.value();
            assert(it.value() >= 0);
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


    std::mutex mut;

    /** Defaults to 127.0.0.1. Must be changed when needed. */
    std::string d_server_ip;
    TagsMap m_tags;

private:
    PortsMap m_free;
    PortsMapRef m_map_ref;
    uint16_t low, hi;

};

RTPPortPair::~RTPPortPair()
{
    if (nullptr != dsp && valid())
    {
        reset();
    }
}

ZConstString RTPPortPair::dispensers_ip() const
{
   return ZConstString(dsp->d_server_ip.data(), dsp->d_server_ip.size());
}

void RTPPortPair::reset()
{
   dsp->release(*this);
   even = 0;
   odd = 0;
   memset(tag, 0x00, sizeof(tag));
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
    std::lock_guard<std::mutex> lk(pv->mut);
    pv->d_server_ip = addr;
}

ZConstString RTPPortsDispencer::dest_ip() const
{
    std::lock_guard<std::mutex> lk(pv->mut);
    return ZConstString(pv->d_server_ip.data(), pv->d_server_ip.size());
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
