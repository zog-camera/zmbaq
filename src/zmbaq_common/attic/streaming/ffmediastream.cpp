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


#include "ffmediastream.h"
#include "ffmpeg_rtp_encoder.h"
#include "Poco/Message.h"
#include "../ffilewriter.h"
//#include "../detection/bgsegmenter.h"




extern "C" {
#include <libavutil/pixfmt.h>
#include <assert.h>
}


//void ffmediastream::on_surface_frame_available(SHP(QImage) frame)
//{
//    if (d_block || nullptr == pv)
//        return;

//    frame_size = frame->size();
////    int w = frame_size.width();
////    int h = frame_size.height();
////    if (w > 1280)
////    {
////        float r = w;
////        r /= h;
//////        bool ratio_4d3 = 1.32 < r && r < 1.70;
////        bool ratio_16d9 = 1.79 > r && r > 1.34;

////        w = 1280;
////        h = ratio_16d9? 960 : 720;
////    }
//    if (!pv->valid() || pv->rtp_ip() != d_server_ip.get_const_str() )
//    {
//        //it will do nothing if already created:
//        pv->create_stream(d_server_ip, d_port,
//                          frame_size.width(), frame_size.height());
//    }

//    if (pv->valid() && pv->push_data(frame->bits(), frame->size(), AV_PIX_FMT_RGB24))
//        pv->send_data();
//}

//void ffmediastream::on_frame_available(SHP(SurvFrame) frame)
//{
//    on_surface_frame_available(frame->rgb_frame);
//}

namespace ZMB {

void ffmediastream::block_stream()
{
    d_block = true;
}

void ffmediastream::unblock_stream()
{
    d_block = false;
}


//***************************************************************************
//***************************************************************************
//***************************************************************************



ffmediastream::ffmediastream(Poco::Channel* logChannel) : d_port(4999), d_block(false)
{
  d_str_port.fill(0x00);
  snprintf(d_str_port.data(), d_str_port.size(), "%u", d_port);
  this->logChannel = logChannel;
}

ffmediastream::ffmediastream(const AVFormatContext* src_context,
                             const Poco::Net::IPAddress& remote_addr,
                             u_int16_t udp_port, Poco::Channel* logChannel)
  : ffmediastream(logChannel)
{
  pv = std::make_shared<ffmpeg_rtp_encoder>();
  auto addr_str = remote_addr.toString();
  pv->create_stream(src_context, ZConstString(addr_str.data(), addr_str.size()), udp_port);
}

ffmediastream::~ffmediastream()
{
    if (nullptr != pv)
    {
        pv->destroy_stream();
    }
}

bool ffmediastream::is_connected() const
{
    return (nullptr != pv);
}

void ffmediastream::stream_to_remote(const ZConstString& ip, u_int16_t udp_port)
{
    d_port = udp_port;
    snprintf (d_str_port.data(), d_str_port.size(), "%u", d_port);

    d_server_ip = ip;

    if (nullptr == pv)
    {
        pv = std::make_shared<ffmpeg_rtp_encoder>();
        pv->sidedata = (void*)this;
    }
    //Notice:
    //-- ffmpeg_rtp_encoder will be started only when first input image comes.
    //-- Until then it won't be initialized

    if (nullptr != cb_failed)
    {
        pv->cb_failed = [&] (const ffmpeg_rtp_encoder*, const std::string& msg)
        {
            if (logChannel)
              {
                logChannel->open();
                Poco::Message _msg;
                _msg.setSource(__FUNCTION__);
                _msg.setText(msg);
                logChannel->log(_msg);
              }
            ffmediastream::cb_failed(this, msg);
        };
    }
    if (nullptr != cb_stream_created)
    {
        pv->cb_stream_created = [&](const ffmpeg_rtp_encoder* encod, SHP(Json::Value) rtp_set) mutable
        {
            if (logChannel)
              {
                logChannel->open();
                Poco::Message _msg;
                _msg.setSource(__FUNCTION__);
                _msg.setText(std::string("ffmpeg_stream spawned"));
                logChannel->log(_msg);
              }
            cb_stream_created(this, rtp_set);
        };
    }

}

void ffmediastream::stream_to_localhost(u_int16_t udp_port)
{
    stream_to_remote(ZCSTR("127.0.0.1"), udp_port);
}

bool ffmediastream::send_packet(std::shared_ptr<AVPacket> pkt)
{
    return pv->send_packet(pkt);
}

const ZConstString ffmediastream::port_str() const
{
    return ZConstString(d_str_port.data(), d_str_port.size());
}

u_int16_t ffmediastream::port() const
{
    return d_port;
}

ZConstString ffmediastream::server() const
{
    return d_server_ip.get_const_str();
}

glm::ivec2 ffmediastream::encoded_frame_size() const
{
    return frame_size;
}

SHP(ZMB::FFileWriterBase) ffmediastream::write_file(const ZConstString& fname)
{
    curFileRef = (nullptr != pv)? pv->write_file(fname) : nullptr;
    return curFileRef;
}

/** Stop dump to file.*/
SHP(ZMB::FFileWriterBase) ffmediastream::stopFile()
{
    if (nullptr != pv)
    {
        pv->stop_file();
        if (nullptr != cb_file_written) cb_file_written(this, curFileRef);
    }
    return curFileRef;
}

std::string ffmediastream::get_sprop() const
{
    return std::move(pv->get_sprop());
}

}//namespace ZMB
