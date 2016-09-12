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


#include "zmb_str.h"
#include <cstring>
#include <assert.h>
#include <arpa/inet.h>

namespace ZMBCommon {

ZConstString bindZCTo(const std::string& str)
{
  return ZConstString(str.data(), str.size());
}

//=========================================
ZConstString::ZConstString(const ZConstString& key, const Json::Value* jobject)
    : ZConstString((const char*)0, (const char*)0)
{
    const Json::Value* cval = jobject->find(key.begin(), key.end());
    if (nullptr != cval)
    {
        const char* b = 0, *e = 0;
        cval->getString(&b, &e);
        this->str = b;
        this->len = (e - b);
    }
    type = Type::UNSPECIFIED;
}
void* ZConstString::write(void* dest, size_t n_bytes) const
{ return memcpy(dest, str, n_bytes == std::string::npos? len : n_bytes);}


ZConstString::ZConstString(const char* a_str)
    : str(a_str), len(nullptr == a_str? 0u : strlen(a_str)) {    type = Type::UNSPECIFIED; }

ZConstString::ZConstString(const char* a_str, size_t a_len, Type tp)
    : str(a_str), len(a_len), type(tp)
{  }

ZConstString::ZConstString(const char* begin, const char* end, Type tp)
    : str(begin), len(end - begin), type(tp)
{ assert(end >= begin); }

bool ZConstString::is_key_of(const Json::Value* jval, const Json::Value** p_jvalue) const
{
    auto jp = jval->find(begin(), end());
    if (nullptr != p_jvalue)
    {
        *p_jvalue = jp;
    }
    return nullptr != jp;
}

bool ZConstString::has_prefix(const ZConstString& prefix_str) const
{
    if (size() < prefix_str.size())
        return false;
    return 0 == memcmp(begin(), prefix_str.begin(), prefix_str.size());
}

//=========================================

ZUnsafeBuf::ZUnsafeBuf() : str(0), len(0), sidedata(0)
{ }

ZUnsafeBuf::ZUnsafeBuf(char* a_str)
    : str(a_str), len(strlen(a_str)), sidedata(0)
{ }

ZUnsafeBuf::ZUnsafeBuf(char* a_str, size_t a_len)
    : str(a_str), len(a_len), sidedata(0)
{ }

ZUnsafeBuf::ZUnsafeBuf(char* b, char* e)
    : str(b), len(e - b), sidedata(0)
{
   assert(b < e);
}

ZUnsafeBuf::~ZUnsafeBuf()
{
}

ZConstString ZUnsafeBuf::to_const() const
{
   return ZConstString(const_cast<char*>(begin()), size());
}

bool ZUnsafeBuf::is_equal(const ZConstString& other) const
{
   if (other.size() != size())
       return false;
   return 0 == memcmp(begin(), other.begin(), other.size());
}

bool ZUnsafeBuf::has_prefix(const ZConstString& prefix_str) const
{
    if (size() < prefix_str.size())
        return false;
    return 0 == memcmp(begin(), prefix_str.begin(), prefix_str.size());
}

void ZUnsafeBuf::imbue(const ZConstString& other, char *dest_start)
{
    if (size() < (other.size() + (nullptr == dest_start? 0u : dest_start - begin()))
            || begin() == other.begin())
        throw std::runtime_error(__PRETTY_FUNCTION__);
    char* e = 0;
    read(&e, other, dest_start);
    this->len = e - begin();
}

#define ZU_COPY_BODY_PART(ITEM_TYPE, FUNC_PTR) {  \
    static const size_t _t_size = sizeof(ITEM_TYPE);                          \
    ITEM_TYPE* start = (NULL == start_pos)? (ITEM_TYPE*) begin() : start_pos; \
    size_t u_len = data.size() / _t_size;                                \
    assert ((ITEM_TYPE*)end() >= (start + u_len));                 \
    (FUNC_PTR)(start, data.begin(), u_len * _t_size);                      \
    if (0 != end_ptr)                                                    \
        *end_ptr = start + u_len;    }

static const char hex_table[] = "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f404142434445464748494a4b4c4d4e4f505152535455565758595a5b5c5d5e5f606162636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e7f808182838485868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9fa0a1a2a3a4a5a6a7a8a9aaabacadaeafb0b1b2b3b4b5b6b7b8b9babbbcbdbebfc0c1c2c3c4c5c6c7c8c9cacbcccdcecfd0d1d2d3d4d5d6d7d8d9dadbdcdddedfe0e1e2e3e4e5e6e7e8e9eaebecedeeeff0f1f2f3f4f5f6f7f8f9fafbfcfdfeff";

bool ZUnsafeBuf::transform_hex4b(char** end_ptr, const ZConstString& data, char* start_pos)
{
    unsigned n = data.size() / sizeof(u_int32_t);

    char* st =  (0u == start_pos)? begin() : start_pos;
    if ((end() - st) < (6 * n))
        // 6 == sizeof(uint) + 2
        return false;

    memset(st, 0, n * 6);

    {
        char* p = st;
        const u_int32_t* s = (const u_int32_t*)data.begin();
        register u_int32_t diff = 0;
        register u_int8_t byte = 0;
        for (unsigned c = 0; c < n; ++c, p += (2 + 8), ++s)
        {
            p[0] = '0'; p[1] = 'x';
            const u_int32_t& val (*s);
            for (char* pp = p + 2; pp < p + 10; ++pp)
            {
                diff = pp - (p + 2);
                byte = 0xff & (val >> (4 * diff));
//                byte = ((0xff000000 & (val << 8 * diff)) >> 8 * diff);
                *(  pp) = hex_table[byte * 2    ];
                *(++pp) = hex_table[byte * 2 + 1];
            }
        }
    }
    *end_ptr = st + n * 6;
    return true;
}

bool ZUnsafeBuf::from_hex4b(char** end_ptr, const ZConstString& hex_data, char* start_pos)
{
    const char* b = hex_data.begin();
    for (; b < hex_data.end() && *b != '0'; ++b)
    { /*seek for '0'*/ }
    if (b == hex_data.end())
        return false;

    ZConstString hex(b, hex_data.end());
    unsigned n = (hex.size() * 4) / 10;
    char* st =  (0u == start_pos)? begin() : start_pos;
    if ((end() - st) >= n)
    {
        return false;
    }
    memset(st, 0, n);

    /** TODO: use register u_int64_t here, instead of (u_int16_t*)words. **/
    u_int16_t* words = (u_int16_t*)(hex.begin() + 2);
    char* dest = begin();

    auto fn_texth16_to_char = [](char& dst, char& v) {
        dst += (v <= '9')? (v - '0') * 16 : (v - 'a') * 16;
    };

    for (u_int32_t c = 0; c < hex.size() / 10; ++c, words += 5, dest += 4)
    {
        //process 4 words:
        for (u_int8_t q = 0; q < 4; ++q)
        {
            register char val = words[q] & 0xff;
            fn_texth16_to_char(dest[2 * q    ], val);
            val = (0xff00 & words[q]) >> 8;
            fn_texth16_to_char(dest[2 * q + 1], val);
        }
    }
    *end_ptr = st + n;
    return true;
}

bool ZUnsafeBuf::read(char** end_ptr, const ZConstString& data, char* start_pos)
{
    ZU_COPY_BODY_PART(char, &memcpy);
    return true;
}

bool ZUnsafeBuf::read_htonl(u_int32_t** end_ptr, const ZConstString& data, u_int32_t* start_pos)
{
    ZU_COPY_BODY_PART(u_int32_t, &memcpy);
    return htonl_self(start_pos, *end_ptr);
}

bool ZUnsafeBuf::read_htons(u_int16_t** end_ptr, const ZConstString& data, u_int16_t* start_pos)
{
    ZU_COPY_BODY_PART(u_int16_t, &memcpy);
    return htons_self(start_pos, *end_ptr);
}

bool ZUnsafeBuf::read_ntohl(u_int32_t** end_ptr, const ZConstString& data, u_int32_t* start_pos)
{
    ZU_COPY_BODY_PART(u_int32_t, &memcpy);
    return ntohl_self(start_pos, *end_ptr);
}

bool ZUnsafeBuf::read_ntohs(u_int16_t** end_ptr, const ZConstString& data, uint16_t* start_pos)
{
    ZU_COPY_BODY_PART(u_int16_t, &memcpy);
    return ntohs_self(start_pos, *end_ptr);
}
#undef ZU_COPY_BODY_PART

#define HN_CONVERT_BODY(TYPE, FUNC_NAME) {                   \
    TYPE* p_begin = (TYPE*)begin();                          \
    TYPE* p_end = begin_ptr;                                 \
    p_end += size() / sizeof(TYPE);                          \
    if (begin_ptr <  p_begin || begin_ptr > p_end) { return false; }\
    if (0 != end_ptr)                                        \
    {                                                        \
        if (end_ptr > p_end || end_ptr < p_begin){ return false; } \
        p_end = end_ptr;                                     \
    }                                                        \
    for (TYPE* item = begin_ptr; item < p_end; ++item)       \
    {                                                        \
        *item = (FUNC_NAME)(*item);                          \
    }                                                        \
    return true; }



    //htonl() for a region of buf;
bool ZUnsafeBuf::htonl_self(u_int32_t* begin_ptr, u_int32_t* end_ptr)
{
    HN_CONVERT_BODY(u_int32_t, htonl);
    return false;
}

bool ZUnsafeBuf::htons_self(u_int16_t* begin_ptr, u_int16_t* end_ptr)
{
    HN_CONVERT_BODY(u_int16_t, htons);
    return false;
}

bool ZUnsafeBuf::ntohl_self(u_int32_t* begin_ptr, u_int32_t* end_ptr)
{
    HN_CONVERT_BODY(u_int32_t, ntohl);
    return false;
}

bool ZUnsafeBuf::ntohs_self(u_int16_t* begin_ptr, u_int16_t* end_ptr)
{
    HN_CONVERT_BODY(u_int16_t, ntohs);
    return false;
}

void ZUnsafeBuf::fill(int val) {if (size() > 0) memset(begin(), val, size());}

}//namespace ZMBCommon
///----------------------------------------------------------------------------
bool operator == (const ZMBCommon::ZConstString& first, const ZMBCommon::ZConstString& second)
{
    return first.begin() == second.begin() && first.size() == second.size();
}

bool operator != (const ZMBCommon::ZConstString& first, const ZMBCommon::ZConstString& second)
{
    return !(first == second);
}
///----------------------------------------------------------------------------

