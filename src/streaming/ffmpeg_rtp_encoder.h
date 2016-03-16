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


#ifndef FFMPEG_RTP_ENCODER_H
#define FFMPEG_RTP_ENCODER_H

extern "C" {
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libavutil/rational.h>
}
#include <functional>
#include "zmbaq_common.h"
#include "timingutils.h"
#include "mbytearray.h"
#include "jsoncpp/json/json.h"
#include "glm/vec2.hpp"


struct AVFrame;
struct SwsContext;
struct AVOutputFormat;
struct AVFormatContext;
struct AVStream;
struct AVCodec;
struct AVPicture;
struct AVRational;
struct AVPacket;
struct AVCodecContext;
struct AVDictionary;

namespace ZMB {
    class FFileWriterBase;
}

namespace av {
 class Packet;
}

class ffmpeg_rtp_encoder;

void  log_packet(AVRational* time_base, const AVPacket *pkt);

class PacketsPocket
{
public:

    /** pair(Time in seconds, pts in time_base) */
    struct seq_key_t : public std::pair<double, int64_t>
    {
    public:
        seq_key_t() {first = 0.0; second = 0u;}
        seq_key_t(const double& s, const int64_t& pts) {first = (s); second = (pts); }
        const double& seconds() const {return first;}
        const int64_t& pts() const {return second;}
    };

    typedef std::pair<seq_key_t, SHP(av::Packet)> marked_pkt_t;
    typedef std::deque<marked_pkt_t, tbb::cache_aligned_allocator<marked_pkt_t>> packet_list_t;
    typedef std::pair<seq_key_t, packet_list_t> seq_item_t;

    PacketsPocket();
    ~PacketsPocket();

    void push(AVPacket* pkt, SHP(ZMB::FFileWriterBase) pfile, const AVRational& time_base);
    void dump_packets(seq_key_t& key, SHP(ZMB::FFileWriterBase)& pfile);

    /** Frame sequences map. First frame in each sequence is a keyframe.*/
    std::deque<seq_item_t, tbb::cache_aligned_allocator<seq_item_t>> seq;

    /** Reset for each new file:*/
    TimingUtils::LapTimer laptime;
    int64_t prev_pts;
    char msg[256];

    seq_key_t last_dump_head_mark;
    seq_key_t last_dump_tail_mark;

};

/** Encoder AND/OR H264/RTP transmitter.*/
class ffmpeg_rtp_encoder
{
public:
    void* sidedata; //< set and/or managed externally
    std::function<void(const ffmpeg_rtp_encoder*, ZMBCommon::MByteArrayPtr)> cb_failed;
    std::function<void(const ffmpeg_rtp_encoder*, SHP(Json::Value) rtp_settings)> cb_stream_created;


    explicit ffmpeg_rtp_encoder();
    virtual ~ffmpeg_rtp_encoder();

    ZMBCommon::MByteArray get_sprop_parameters(glm::ivec2 encoded_frame_size);

    static glm::ivec2 default_enc_frame_size();

    // create restreamer of already encoded H264 packets from source context:
    int create_stream(const AVFormatContext* src_ctx, ZConstString rtp_ip, u_int16_t rtp_port);

    int create_stream(ZConstString rtp_ip, u_int16_t rtp_port,
                      int enc_width, int enc_height);
    void destroy_stream();

    //transmit already encoded packet:
    bool send_packet(AVPacket* pkt);

    /** Copy frame into internal buffer.
     * The frame shall be converted to appropriate size for the encoder.
     * @return false if stream not created. */
    bool push_data(const u_int8_t* data, glm::ivec2 data_size, int /*AVPixelFormat*/ data_format);

    /** Encoder H264 and amke RTP dispatch*/
    bool send_data();

    /** Check whether it's able to work.
     * @retun false on some errors. */
    bool valid() const;

    ZConstString rtp_ip() const;

    /** Returns S-prop parameters. Non-empty if the stream is created.*/
    ZConstString get_sprop() const;

    //generate this->sprop_parameters
    SHP(Json::Value) make_rtp_settings();
    ZMBCommon::MByteArrayPtr make_rtp_settings_str(const Json::Value& jo);

    SHP(ZMB::FFileWriterBase) cur_file() const;

    /** Write h264 frames to .mp4 file */
    SHP(ZMB::FFileWriterBase) write_file(const ZConstString& fname);

    void stop_file();

private:

    SHP(ZMB::FFileWriterBase) p_file_writer;

    /** Dimensions of the encoded video frame*/
    glm::ivec2 d_encod_size;
    bool f_ready;

    const char *filename;
    ZMBCommon::MByteArray d_rtp_ip;
    u_int16_t d_rtp_port;
    AVOutputFormat *fmt;
    AVFormatContext *oc;
    AVStream *video_st;
    AVCodec  *video_codec;
    double video_time;
    int flush, ret;

    AVDictionary* dict;


    int video_is_eof;


    /* video output */

    SwsContext* sws_ctx;
    AVFrame *frame;
    AVPicture*  dst_picture;
    u_int64_t frame_count;
    u_int64_t bad_frame_count;

    TimingUtils::LapTimer tm;
    //filled on create_stream();
    ZMBCommon::MByteArray sprop_parameters;

    PacketsPocket pock;

private:



    void close_video(AVStream *st);
    bool write_video_frame(AVFormatContext *oc, AVStream *st, int flush);

    void open_video(AVCodec *codec, AVStream *st);
    int        write_frame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt);


    AVStream* add_stream(AVFormatContext* fmt_ctx, AVCodec **codec, int codec_id) const;
    AVStream* make_vstream_w_defaults(AVFormatContext* afctx, AVCodec** codec) const;
    // case AVMEDIA_TYPE_VIDEO; c->codec_id must be preset.
    void set_vcodec_defaults(AVCodecContext* c, AVStream* st, glm::ivec2 encod_size) const;

};

#endif // FFMPEG_RTP_ENCODER_H
