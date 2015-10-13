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


#ifndef SDLGHOLDER_H
#define SDLGHOLDER_H

#include <memory>
#include <Poco/Net/IPAddress.h>
#include "mbytearray.h"
#include "zmbaq_common.h"
#include "jsoncpp/json/json.h"

namespace ZMFS {
    class FSItem;
}


class ffmediastream;
namespace ZMB {
    class FFileWriterBase;
}
struct AVFormatContext;
class ffmpeg_rtp_encoder;
struct AVPacket;

//-----------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
/** Takes video frames from QCamSurface and sends them to the server
 * Currently it streams to local UDP port and than reads that UDP
 * traffic and re-transmits to the remote server.
 *
 *
 * [VideoSurface]---{frame}--->[FFmeg H264/RTP encoder]
 * |
 * ----->[remote server.]
 * .*/
class MediaHandle : public std::enable_shared_from_this<MediaHandle>
{
public:
    void* sidedata;

    /** Emitted when file write has stopped.*/
    std::function<void (SHP(MediaHandle), SHP(ZMB::FFileWriterBase))> sig_file_written;

//    explicit MediaHandle (SurvFfmpegCam* master);
    MediaHandle ();
    virtual ~MediaHandle ();

    SHP(ffmediastream) get_media_stream() const;

    /** Return currently used UDP port on localhost
     * @return 0 if not constructed. */
    u_int16_t port_num() const;

    /** Pass H264 packet from src context to destination RTP(H264).
    @param src_context: Must be previously constructed with same context pointer.
    */
    bool send_rtp_packet(AVPacket* pkt);

    bool is_busy() const;


    /** Dump H264 to .mp4 file (any container of FFmpeg)*/
    SHP(ZMB::FFileWriterBase) write_file(SHP(ZMFS::FSItem) file);

    /** @return pointer to recent file or NULL if called prior to write_file().*/
    SHP(ZMB::FFileWriterBase) stop_file();

//    /** Set up to get raw frames from USB camera and amke H264 encoding & RTP streaming.*/
//    void construct(SHP(SurvFfmpegCam) cam, const QHostAddress& remote_addr, u_int16_t udp_port,
//                   bool direct_connection = false);

    /** Construct re-streamer anything to RTP*/
    void construct(const AVFormatContext* src_context,
                   const Poco::Net::IPAddress& remote_addr, u_int16_t udp_port,
                   bool direct_connection = true);

    /** Temporarly pause the RTP media streaming of the media call,
     * This will cause Call destruction on 5sec timeout if not unblocked.*/
    void block_media_stream();

    /** Restart RTP stream in media call, if called after timeout
     * the it'll create new media call via construct(). */
    void unblock_media_stream();

    void destruct();

    /** Provides and object from which valid SDP is generated.
     * Works only after construct(); */
    const Json::Value& get_sdp() const;

//    /** pass frame to the h264/RTP encoder and perform the encoding.
//     * Move this object to separate thread to make this operation
//     * asynchronous.
//     * @param fram: raw image to be encoded. */
//    void on_surface_frame_available(SHP(QImage) frame);

//    /** pass frame to the h264/RTP encoder and perform the encoding.
//     * Move this object to separate thread to make this operation
//     * asynchronous.
//     * @param fram: raw image to be encoded. */
//    void on_frame_available(SHP(SurvFrame) frame);


    void on_file_writer_error(std::shared_ptr<ZMB::FFileWriterBase> fptr,
                              const ZMBCommon::MByteArray& msg);

    void on_rtp_stream_created(const Json::Value& rtp_settings, const ZMBCommon::MByteArray& rtp_settings_str);

protected:
    void bind_callbacks(SHP(ffmediastream) ffstream, bool direct_connection);

    //keeps the current sdp settings:
    Json::Value sdp;
    ZMBCommon::MByteArray sdp_str;

    SHP(ffmediastream) media_stream;
//    SHP(SurvFfmpegCam) p_cam;
//    SurvFfmpegCam* p_master;
};
//------------------------------------------------------------------------------------
typedef std::shared_ptr<MediaHandle> MediaHandlePtr;
//------------------------------------------------------------------------------------

#endif // SDLGHOLDER_H
