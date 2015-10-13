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


#include "mediahandle.h"

#include "ffmediastream.h"
#include "ffmpeg_rtp_encoder.h"
#include "logservice.h"
#include "../ffilewriter.h"
#include "../fshelper.h"


//-----------------------------------------------------
//#include "../qcamsurface.h"
//-----------------------------------------------------
//#include "../survffmpegcam.h"

MediaHandle::MediaHandle ()
{
   sidedata = 0;
}
//MediaHandle::MediaHandle (SurvFfmpegCam* master)
//{
//    p_master = master;
//}

MediaHandle::~MediaHandle ()
{
    destruct();
}

bool MediaHandle::is_busy() const
{
   return false;
}

std::shared_ptr<ffmediastream> MediaHandle::get_media_stream() const
{
    return media_stream;
}

u_int16_t MediaHandle::port_num() const
{
    return (nullptr == media_stream)? 0 : media_stream->port();
}

void MediaHandle::destruct()
{
//    p_master->unbind_handle(this);
    if (NULL != media_stream)
    {
        media_stream->block_stream();
        //unref:
        std::move(media_stream);
    }
}

void MediaHandle::bind_callbacks(SHP(ffmediastream) ffstream, bool direct_connection)
{
    media_stream->cb_stream_created = [&](ffmediastream* ff, SHP(Json::Value) jsonptr) mutable
    {
        sdp = *jsonptr;
        sdp_str = ZMBCommon::jexport(jsonptr.get());
    };

    media_stream->cb_file_written = [&](ffmediastream* ffr, SHP(ZMB::FFileWriterBase) file)
    {
        sig_file_written(shared_from_this(), file);
    };

    media_stream->cb_failed = [&](ffmediastream* ff, ZMBCommon::MByteArrayPtr msg) mutable
    {
        LERROR(ZCSTR("MediaHandle"), msg);
        destruct();
    };
}

//void MediaHandle::construct(SHP(SurvFfmpegCam) cam, const QHostAddress& remote_addr,
//                            u_int16_t udp_port, bool direct_connection)
//{
//    ADEBUG(ZCSTR(__PRETTY_FUNCTION__));
//    media_stream = std::make_shared<ffmediastream>();
//    bind_callbacks(media_stream, direct_connection);
//    ZMBCommon::MByteArray addr(remote_addr.toString());
//    media_stream->stream_to_remote(addr.get_const_str(), udp_port);
//    Qt::ConnectionType ctype = direct_connection? Qt::DirectConnection : Qt::QueuedConnection;

//    QObject::connect(cam.get(), &SurvFfmpegCam::sig_got_frame,
//                     this, &MediaHandle::on_frame_available, ctype);
//}

void MediaHandle::construct(const AVFormatContext* src_context,
                            const Poco::Net::IPAddress& remote_addr,
                            u_int16_t udp_port, bool direct_connection)
{
    ADEBUG(ZCSTR(__PRETTY_FUNCTION__));
    media_stream = std::make_shared<ffmediastream>(src_context, remote_addr, udp_port);
    bind_callbacks(media_stream, direct_connection);
    ZMBCommon::MByteArray addr(remote_addr.toString());
    media_stream->stream_to_remote(addr.get_const_str(), udp_port);
}

void MediaHandle::on_rtp_stream_created(const Json::Value& rtp_settings, const ZMBCommon::MByteArray& rtp_settings_str)
{
    sdp = rtp_settings;
    sdp_str = rtp_settings_str;
}

bool MediaHandle::send_rtp_packet(AVPacket* pkt)
{
    if (nullptr != media_stream)
        return media_stream->send_packet(pkt);
    return false;
}

void MediaHandle::block_media_stream()
{
    media_stream->block_stream();
}

void MediaHandle::unblock_media_stream()
{
    bool iam_late = (NULL == media_stream);
    if (iam_late)
    {
        //re-construct the stream:
//        if (nullptr != p_cam)
//        {
//            ZConstString addr = media_stream->server();
//            construct(p_cam, QHostAddress(QLatin1String(addr.begin(), addr.size())), media_stream->port());
//        }
    }
    else
    {
        media_stream->unblock_stream();
    }
}

SHP(ZMB::FFileWriterBase) MediaHandle::write_file(SHP(ZMFS::FSItem) file)
{
    assert(nullptr != file);
    ADEBUG(ZCSTR(__PRETTY_FUNCTION__));
    if (nullptr != media_stream)
    {
        std::string dest;
        file->fslocation->absolute_path(dest, file->fname.to_const());
        auto fwriter = media_stream->write_file(ZConstString(dest.data(), dest.size()));
        assert(nullptr != fwriter);
        fwriter->fs_item = file;
        return fwriter;
    }
    else
    {
        AERROR(ZCSTR(__PRETTY_FUNCTION__));
        //return empty writer:
        return std::make_shared<ZMB::FFileWriterBase>();
    }
}

SHP(ZMB::FFileWriterBase) MediaHandle::stop_file()
{
    ADEBUG(ZCSTR(__PRETTY_FUNCTION__));
    if (nullptr != media_stream)
    {
        auto fw = media_stream->stop_file();
        sig_file_written(shared_from_this(), fw);
        return fw;
    }
    return std::shared_ptr<ZMB::FFileWriterBase>(nullptr);
}

//void MediaHandle::on_surface_frame_available(SHP(QImage) frame)
//{
//   media_stream->on_surface_frame_available(frame);
//}

//void MediaHandle::on_frame_available(SHP(SurvFrame) frame)
//{
//   media_stream->on_frame_available(frame);
//}

void MediaHandle::on_file_writer_error(std::shared_ptr<ZMB::FFileWriterBase> fptr,
                                       const ZMBCommon::MByteArray& msg)
{
//    Q_UNUSED(fptr);
    AERROR(msg);
}

const Json::Value& MediaHandle::get_sdp() const
{
   return sdp;
}
