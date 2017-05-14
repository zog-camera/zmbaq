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


#include "ffilewriter.h"
#include <string>


namespace ZMB {

//===============================================================================

//-------------------------------
FFileWriter::FFileWriter()
{
  is_open = false;
  pb_file_size.store(0u);
}

FFileWriter::FFileWriter(const AVFormatContext *av_in_fmt_ctx, const std::string& dst)
{
    open(av_in_fmt_ctx, dst);
}

FFileWriter::~FFileWriter()
{
    close();
}

bool FFileWriter::open(const AVFormatContext *av_in_fmt_ctx, const std::string& dst)
{
    pv = std::make_shared<PV>(this, av_in_fmt_ctx, dst);
    is_open = pv.open();
    if (!is_open)
        pv.reset();
    return is_open;
}


void FFileWriter::write(std::shared_ptr<av::Packet> pkt)
{
   pv.write(pkt);
}

void FFileWriter::close()
{
    if (is_open)
        pv.close();
    is_open = false;
}

void FFileWriter::dumpPocketContent(ZMB::PacketsPocket::seq_key_t lastPacketTs)
{
  pocket.dump_packets(lastPacketTs, this);
}

}//ZMB

