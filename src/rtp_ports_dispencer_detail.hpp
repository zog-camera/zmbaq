#include <map>
#include <array>
#include <cassert>

namespace ZMB {


//----------------------------------------------------------

/** Map<uint16_t(even_port), uint16_t(odd_port)> */
typedef  std::map<uint16_t, uint16_t> PortsMap;
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
	assert(str_8bytes.size() >= 8);
	num = * static_cast<u_int64_t*>(srt_8bytes.begin());

        EvensArrayPtr array;
        auto it = find(num);
        array = (end() == it)? (*it).second : std::make_shared<EvensArray>();
        return array;
    }

    //push tagged RTP even port
    void tag(const ZConstString& str_8bytes, uint16_t even_port)
    {
        u_int64_t num = 0;
	assert(str_8bytes.size() >= 8);
	num = * static_cast<u_int64_t*>(srt_8bytes.begin());

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
	assert(str_8bytes.size() >= 8);
	num = * static_cast<u_int64_t*>(srt_8bytes.begin());

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
	assert(str_8bytes.size() >= 8);
	num = * static_cast<u_int64_t*>(srt_8bytes.begin());

	auto it = find(num);
	assert(num > 0 && end() != it); //avoid situations of improper user
	
	auto arrayp = (*it).second;
	
	auto array_iter = arrayp->begin();
	for (auto& val : *arrayp)
	  {
	    if (port != val) { ++array_iter; continue;  }
	    arrayp->erase(array_iter);
	    break;
	  }
    }
};

//------

/** Keeps lists of used RTP/RTCP ports.*/
class RTPPortsDispencerPV
{
public:
    RTPPortsDispencerPV(uint16_t rlow, uint16_t rhi)
        : low(rlow), hi(rhi)
    {
        bool ok = true;
        //insert (even, odd) pairs
        for (uint16_t c = rlow; c < rhi && ok; c += 2)
        {
            m_free[c] = c + 1;
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
	if (m_free.end() == iter)
	  return RTPPortPair();
	even = (*iter).first;
	odd = (*iter).second;
	//same as this->incref(even);
	m_free.erase(iter);
	
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

    void release(const RTPPortPair& item)
    {
      m_tags.remove_tagged_port(ZConstString(item.tag.data(),item.tag.size()), item.even);
      m_free[item.even] = item.odd;
    }


    /** Defaults to 127.0.0.1. Must be changed when needed. */
    std::string d_server_ip;
    TagsMap m_tags;
    PortsMap m_free;
    uint16_t low, hi;

};
