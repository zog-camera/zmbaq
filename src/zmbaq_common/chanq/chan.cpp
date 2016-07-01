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


#include "chan.h"

ChanCtrl::ChanCtrl()
{
    sidedata = 0;
}

void ChanCtrl::make_sig_closed()
{
    if (0 != sig_closed)
        sig_closed(shared_from_this());
}

void ChanCtrl::make_sig_opened()
{
    if (0 != sig_opened)
        sig_opened(shared_from_this());
}

//case cant alloc:
void ChanCtrl::make_sig_bad_alloc()
{
    if (0 != sig_bad_alloc)
    {
        sig_bad_alloc(shared_from_this());

        if (0 != sig_error)
        {
            ZMBCommon::MByteArray err("Not enough of memory!");
            err += ZMBCommon::MByteArray(__PRETTY_FUNCTION__);
            sig_error(shared_from_this(), err);
        }
    }
}

//when pushed above the limit:
void ChanCtrl::make_sig_overflow(size_t n_items_count)
{
    if (0 != sig_overflow)
    {
        sig_overflow(shared_from_this(), n_items_count);
        if (0 != sig_error)
        {
            ZMBCommon::MByteArray err("Overflow!");
            err += ZMBCommon::MByteArray(__PRETTY_FUNCTION__);
            sig_error(shared_from_this(), err);
        }
    }
}
