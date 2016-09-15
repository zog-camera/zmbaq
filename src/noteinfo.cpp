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


#include "noteinfo.h"
#include <arpa/inet.h>
#include "fshelper.h"
#include <cstring>

namespace ZMB {

char* to_texthex_4b(ZUnsafeBuf* dest, const ZConstString& data)
{
    char* end = 0;
    if (dest->size() < (data.size() / 4) * 10)
        return end;
    dest->transform_hex4b(&end, data);
    return end;
}

char* from_texthex_4b(ZUnsafeBuf* data, const ZConstString& hex_string)
{
    char* end = 0;
    if (data->size() < (hex_string.size() * 4) / 10)
        return end;
    data->from_hex4b(&end, hex_string);
    return end;
}

size_t estimate_size_in_hex4b(size_t data_len)
{
    return (data_len * 10) / 4;
}

size_t estimate_size_from_hex4b(size_t hex_len)
{
    return (hex_len * 4) / 10;
}

bool NoteInfo::NoteId::valid() const
{
   return ts > 0;
}

bool NoteInfo::NoteId::make_event_id(ZUnsafeBuf* str, char** p_end) const
{
    // htons() video_eneity_i and copy:
    ZUnsafeBuf txt(str->begin(), str->size());
    char* end = video_entity_id.make_entity_id(&txt);
    if (0 == end)
        return false;

    txt.len = end - txt.end();
    txt.str = end;
    // htons() ts and copy:
    u_int64_t t = 0;
    u_int32_t* p_ts = (u_int32_t*)&ts;
    t = htonl(p_ts[0]);
    t << 32;
    t |= htonl(p_ts[1]);

    ::memcpy(txt.end(), &t, sizeof(t));
    end += sizeof(t);

    str->len = end - str->begin();
    if (0 != p_end)
        *p_end = end;
    return true;
}

bool NoteInfo::NoteId::read_from_hex4b(const ZConstString& hex4b)
{
    char* end = 0;
    bool ok = video_entity_id.read_from_hex4b(hex4b, &end);
    if (!ok)
        return false;

    ZConstString substr(end, (size_t)(hex4b.end() - end));
    ZUnsafeBuf dt((char*)&ts, sizeof(ts));
    end = ZMB::from_texthex_4b(&dt, substr);
    dt.ntohl_self((u_int32_t*)dt.begin());
    ok = ok && !(0 == end || (end - (char*)this) <= sizeof(*this));
    return ok;
}

bool NoteInfo::NoteId::operator () (const NoteInfo::NoteId& one, const NoteInfo::NoteId& two) const
{
    if (one.ts < two.ts)
        return true;
    return one.video_entity_id(one.video_entity_id, two.video_entity_id);
//    return one.video_entity_id < two.video_entity_id;
}

}//ZMB

bool operator < (const ZMB::NoteInfo::NoteId& one, const ZMB::NoteInfo::NoteId& two)
{
   return one.operator ()(one, two);
}
