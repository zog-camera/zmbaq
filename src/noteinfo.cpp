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
#include <cstring>

namespace ZMB {

bool NoteInfo::NoteId::valid() const
{
   return ts > 0;
}

bool NoteInfo::NoteId::operator () (const NoteInfo::NoteId& one, const NoteInfo::NoteId& two) const
{
    if (one.ts < two.ts)
        return true;
    return one.video_entity_id(one.video_entity_id, two.video_entity_id);
}

}//ZMB

bool operator < (const ZMB::NoteInfo::NoteId& one, const ZMB::NoteInfo::NoteId& two)
{
   return one.operator ()(one, two);
}
