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


#ifndef NOTEINFO_H
#define NOTEINFO_H
#include <Poco/DateTime.h>
#include "zmbaq_common/zmbaq_common.h"
#include "survcamparams.h"

namespace ZMFS {
    class FSItem;
}
class VideoEntity;

namespace ZMB {


ZM_DEF_STATIC_CONST_STRING(NOTE_KW_DATE, "date")
ZM_DEF_STATIC_CONST_STRING(NOTE_KW_TIME, "time")
ZM_DEF_STATIC_CONST_STRING(NOTE_KW_TIMESEC, "timesec")
ZM_DEF_STATIC_CONST_STRING(NOTE_KW_TIMEZONE, "timezone")
ZM_DEF_STATIC_CONST_STRING(NOTE_KW_DURATION, "duration") // floating point format: 1.23

/** Process input data by 4 bytes and print it's hex values.
 *  @param dest: must point to existing location of size (data.size() * 2.5f)
 *  @return end pointer of the (dest), next char item after the last one.
*/
char* to_texthex_4b(ZUnsafeBuf* dest, const ZConstString& data);

/** Gather data hexademical values, each hex value must represent (u_int32_t).
 *  @param data: a buffer of size hex_string.size / 2.5f.
 *  @return end pointer of the (data), next char item after the last one.
*/
char* from_texthex_4b(ZUnsafeBuf* data, const ZConstString& hex_string);

/** @return encoded hex length.*/
size_t estimate_size_in_hex4b(size_t data_len);

/** @return decoded data length.*/
size_t estimate_size_from_hex4b(size_t hex_len);

struct NoteInfo
{
    NoteInfo() { }

    struct NoteId {
        static const u_int8_t fill_byte = 0xff;
        NoteId() {ts = 0;}

        ZMB::VideoEntityID video_entity_id;
        u_int64_t ts;         //< made from dt

        /** Print fields in hex numbers grouped by 4bytes.
         * @param str: must have length >= sizeof(NoteId) * 2.25f;
         *  the length value str->len will be modified.
         *  @param p_end: (p_end[0]) is the end of the string(next element aftert he last one). */
        bool make_event_id(ZUnsafeBuf* str, char** p_end = 0) const;

        /** Read values from hex format of type: 0xaabbccdd (represents u_int32_t)*/
        bool read_from_hex4b(const ZConstString& hex4b);

        bool operator () (const NoteId& one, const NoteId& two) const;

        bool valid() const;


    } noteid;


    Poco::DateTime dt;
    SHP(ZMFS::FSItem) video_file;
    SHP(ZMFS::FSItem) screenshot_file;

    SHP(VideoEntity) the_entity; //< emitter of the event

};

}

bool operator < (const ZMB::NoteInfo::NoteId& one, const ZMB::NoteInfo::NoteId& two);

#endif // NOTEINFO_H
