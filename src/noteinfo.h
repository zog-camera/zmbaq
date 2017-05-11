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
#include "survcamparams.h"

namespace ZMFS {
    class FSItem;
}
class VideoEntity;

namespace ZMB {

enum class NOTE_KW {DATE,TIME,TIMESEC,TIMEZON,DURATION,LAST_ENUM_SIZE};
typedef std::pair<NOTE_KW, std::string> NoteKWPair;
 
NoteKWPair&& GetNowKWords()
{
  return std::move({
      NoteKWPair(NOTE_KW_DATE, "date"),
	NoteKWPair(NOTE_KW_TIME, "time"),
	NoteKWPair(NOTE_KW_TIMESEC, "timesec"),
	NoteKWPair(NOTE_KW_TIMEZONE, "timezone"),
	NoteKWPair(NOTE_KW_DURATION, "duration") // floating point format: 1.23
    });
}
 


struct NoteInfo
{
    NoteInfo() { }

    struct NoteId {
        static const u_int8_t fill_byte = 0xff;
      
        NoteId() { ts = 0; }

        ZMB::VideoEntityID video_entity_id;
        u_int64_t ts;         //< made from dt

        bool operator () (const NoteId& one, const NoteId& two) const;

        bool valid() const;


    } noteid;

    Poco::DateTime dt;
};

}

bool operator < (const ZMB::NoteInfo::NoteId& one, const ZMB::NoteInfo::NoteId& two);

#endif // NOTEINFO_H
