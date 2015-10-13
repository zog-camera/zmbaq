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


#ifndef STCHARCHUNK
#define STCHARCHUNK
#include "mbytearray.h"

template<int P_chunk_size = 256, bool P_does_memset = true>
struct StCharChunk
{
    StCharChunk()
    {
        str.str = buf;
        str.len = sizeof(buf);

        if (P_does_memset)
        {
            memset(str.begin(), 0, str.size());
        }
    }
    ~StCharChunk()
    {

    }

    char buf[P_chunk_size];
    ZUnsafeBuf str;

};

#endif // STCHARCHUNK

