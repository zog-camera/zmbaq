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


#ifndef FFMEDIASTREAM_H
#define FFMEDIASTREAM_H
#include <glm/glm/vec2.hpp>
#include <string>
#include "zmbaq_common.h"
#include "mbytearray.h"
#include "jsoncpp/json/json.h"
#include <Poco/Net/IPAddress.h>

//class QCamSurface;
//class SurvFrame;
class ffmpeg_rtp_encoder;
namespace ZMB {
    class FFileWriterBase;
}

struct AVFormatContext;
struct AVPacket;


class ffmediastream
{
public:
    /** The caller must set these up manually.*/
    std::function<void(ffmediastream*, ZMBCommon::MByteArrayPtr)> cb_failed;

    /** Triggered on first input pkt/frame, when the stream is actually starting.*/
    std::function<void(ffmediastream*, SHP(Json::Value))> cb_stream_created;

    /** Triggered when file write has stopped: * on stop_file(); */
    std::function<void(ffmediastream*, SHP(ZMB::FFileWriterBase) fptr)> cb_file_written;

    ffmediastream();

    /** Creates a capture/restream entity for incoming already encoded RTP traffic.*/
    ffmediastream(const AVFormatContext* src_context,
                  const Poco::Net::IPAddress& remote_addr,
                  u_int16_t udp_port);

    virtual ~ffmediastream();

    static int to_av_pixfmt(int frame_fmt);
    bool is_connected() const;

    const ZConstString port_str() const;
    u_int16_t port() const;

    ZConstString server() const;

    /** The encoded frame size value appears only when
     *  first picture comes for encoding. Max resolution: 1280x720*/
    glm::ivec2 encoded_frame_size() const;

    /** Returns S-prop parameters. Non-empty if the stream is created.*/
    ZConstString get_sprop() const;


    void stream_to_remote(const ZConstString& ip, u_int16_t udp_port);
    void stream_to_localhost(u_int16_t udp_port);

    //used when we just retransmit RTP traffic
    bool send_packet(AVPacket* pkt);

    /** Write h264 frames to .mp4 file.
     * @return ZMB::FFileWriterBase with empty() == true. */
    SHP(ZMB::FFileWriterBase) write_file(const ZConstString& fname);

    /** Stop dump to file.
     * @return recent file that was written or NULL if called without write_file(). */
    SHP(ZMB::FFileWriterBase) stop_file();

    void block_stream();
    void unblock_stream();

//    void on_surface_frame_available(SHP(QImage) frame);
//    void on_frame_available(SHP(SurvFrame) frame);


private:

    glm::ivec2 frame_size;
    u_int16_t d_port;
    std::shared_ptr<ffmpeg_rtp_encoder> pv;
    ZMBCommon::MByteArray d_server_ip;
    char d_str_port[6];
    bool d_block;

    //copied from result of start_file(fname);
    SHP(ZMB::FFileWriterBase) p_file;
};


#endif // FFMEDIASTREAM_H
