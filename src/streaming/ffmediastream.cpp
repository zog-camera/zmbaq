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
//#include <QVideoFrame>
//#include <QImage>
#include "ffmpeg_rtp_encoder.h"
#include "../ffilewriter.h"
#include "logservice.h"
//#include "../detection/bgsegmenter.h"
//#include "../qcamsurface.h"




extern "C" {
#include <libavutil/pixfmt.h>
#include <assert.h>
}

//int ffmediastream::to_av_pixfmt(int frame_fmt)
//{
//    int in_pixel_format = -1;
//    switch (frame_fmt)
//    {
//    case QVideoFrame::Format_ARGB32:
//        in_pixel_format = (int)AV_PIX_FMT_ARGB;
//        break;


//    case QVideoFrame::Format_RGB32          :
//        in_pixel_format = (int)AV_PIX_FMT_RGB32;
//        break;

//    case QVideoFrame::Format_RGB24          :
//        in_pixel_format = (int)AV_PIX_FMT_RGB24;
//        break;

//    case QVideoFrame::Format_RGB565         :
//        in_pixel_format = (int)AV_PIX_FMT_RGB565;
//        break;

//    case QVideoFrame::Format_RGB555         :
//        in_pixel_format = (int)AV_PIX_FMT_RGB555;
//        break;

//    case QVideoFrame::Format_BGRA32         :
//        in_pixel_format = (int)AV_PIX_FMT_BGRA;
//        break;


//    case QVideoFrame::Format_BGR32          :
//        in_pixel_format = (int)AV_PIX_FMT_BGR32;
//        break;

//    case QVideoFrame::Format_BGR24          :
//        in_pixel_format = (int)AV_PIX_FMT_BGR24;
//        break;

//    case QVideoFrame::Format_BGR565         :
//        in_pixel_format = (int)AV_PIX_FMT_BGR565;
//        break;

//    case QVideoFrame::Format_BGR555         :
//        in_pixel_format = (int)AV_PIX_FMT_BGR555;
//        break;

//    case QVideoFrame::Format_AYUV444        :
//        in_pixel_format = (int)AV_PIX_FMT_YUVA444P_LIBAV;
//        break;

//    case QVideoFrame::Format_YUV444         :
//        in_pixel_format = (int)AV_PIX_FMT_YUV444P;
//        break;

//    case QVideoFrame::Format_YUV420P   :
//        in_pixel_format = (int)AV_PIX_FMT_YUV420P;
//        break;


//    case QVideoFrame::Format_UYVY      :
//        in_pixel_format = (int)AV_PIX_FMT_UYVY422;
//        break;


//    case QVideoFrame::Format_YUYV     :
//        in_pixel_format = (int)AV_PIX_FMT_YUYV422;
//        break;

//    case QVideoFrame::Format_NV12      :
//        in_pixel_format = (int)AV_PIX_FMT_NV12;
//        break;

//    case QVideoFrame::Format_NV21       :
//        in_pixel_format = (int)AV_PIX_FMT_NV21;
//        break;


//    case QVideoFrame::Format_Y8:
//        in_pixel_format = (int)AV_PIX_FMT_GRAY8;
//        break;

//    case QVideoFrame::Format_Y16:
//        in_pixel_format = (int)AV_PIX_FMT_GRAY16;
//        break;

//    case QVideoFrame::Format_Jpeg:
//        in_pixel_format = (int)AV_PIX_FMT_YUVJ420P;
//        break;

//    default:
//        assert(false);
//        break;
//    }
//    return in_pixel_format;

//}

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



ffmediastream::ffmediastream() : d_port(4999), pv(0), d_block(false)
{
    memset(d_str_port, 0x00, 6);
    snprintf(d_str_port, 6, "%u", d_port);
}

ffmediastream::ffmediastream(const AVFormatContext* src_context,
                             const Poco::Net::IPAddress& remote_addr,
                             u_int16_t udp_port)
    : d_port(4999), pv(std::make_shared<ffmpeg_rtp_encoder>()), d_block(false)
{
    memset(d_str_port, 0x00, 6);
    snprintf(d_str_port, 6, "%u", d_port);
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
    snprintf (d_str_port, 6, "%u", d_port);

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
        pv->cb_failed = [&] (const ffmpeg_rtp_encoder* encod, ZMBCommon::MByteArrayPtr msg) mutable
        {
            LERROR("ALL", msg);
            cb_failed(this, msg);
        };
    }
    if (nullptr != cb_stream_created)
    {
        pv->cb_stream_created = [&](const ffmpeg_rtp_encoder* encod, SHP(Json::Value) rtp_set) mutable
        {
            LINFO(ZCSTR("ffmpeg_stream"), *rtp_set);
            cb_stream_created(this, rtp_set);
        };
    }

}

void ffmediastream::stream_to_localhost(u_int16_t udp_port)
{
    stream_to_remote(ZCSTR("127.0.0.1"), udp_port);
}

bool ffmediastream::send_packet(AVPacket* pkt)
{
    return pv->send_packet(pkt);
}

const ZConstString ffmediastream::port_str() const
{
    return ZConstString(d_str_port, sizeof(d_str_port));
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
    p_file = (nullptr != pv?
                pv->write_file(fname) : std::make_shared<ZMB::FFileWriterBase>(/*empty file rec*/));
    return p_file;
}

/** Stop dump to file.*/
SHP(ZMB::FFileWriterBase) ffmediastream::stop_file()
{
    if (nullptr != pv)
    {
        pv->stop_file();
        if (nullptr != cb_file_written) cb_file_written(this, p_file);
    }
    return p_file;
}

ZConstString ffmediastream::get_sprop() const
{
    return pv->get_sprop();
}
