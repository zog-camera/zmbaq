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


#include "ffmpeg_rtp_encoder.h"

#include "logservice.h"
#include <Poco/DateTime.h>

extern "C" {
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavcodec/avcodec.h>
#include <libavutil/timestamp.h>
#include <libavutil/time.h>
}

#include "../avcpp/packet.h"

#include "../ffilewriter.h"
#include <array>
//#include "../survframeprocessor.h"

#define STREAM_DURATION  INFINITY
#define STREAM_PIX_FMT    AV_PIX_FMT_YUV420P /* default pix_fmt */

extern "C" {

    static int sws_flags = SWS_BICUBIC;
    static char* extradata2psets(AVCodecContext *c);
}

//-----------------------------------------------------------------
void  log_packet(AVRational* time_base, const AVPacket *pkt)
{
    char buf[256];
    memset(buf, 0x00, sizeof(buf));
    ZMBCommon::MByteArrayList msg;
    msg << "pts:" << av_ts_make_string(buf, pkt->pts)
        << " pts_time: "  << av_ts_make_time_string(buf, pkt->pts, time_base)
        << "dts:" << av_ts_make_string(buf, pkt->dts)
        << " dts_time: "  << av_ts_make_time_string(buf, pkt->dts, time_base)
        << "duration:" << av_ts_make_string(buf, pkt->duration)
        << " duration_time: "  << av_ts_make_time_string(buf, pkt->duration, time_base);
    LDEBUG(ZCSTR("FFRTP"), msg);
}

//-----------------------------------------------------------------
PacketsPocket::PacketsPocket()
{
    prev_pts = 0;
    memset(msg, 0x00, sizeof(msg));
}

PacketsPocket::~PacketsPocket()
{

}


void PacketsPocket::dump_packets(seq_key_t& key, SHP(ZMB::FFileWriterBase)& pfile)
{
    /* Write the compressed frame to the media file. */
    if (nullptr == pfile.get() || pfile->empty())
        return;
    auto it = seq.begin();
    seq_key_t mark = (*it).first;
    //seems that we have wrote part of the sequence (staring with a keyframe) last time
    //lets finish it first:
    if (mark.first == last_dump_head_mark.first)
    {
        packet_list_t& deq ((*it).second);
        snprintf(msg, sizeof(msg),
                "file: %s frames (partial)sequence with %lu frames."
                     "Head (timesec: %lf, pts: %lu)",
                pfile->path().data(), deq.size(),
                last_dump_head_mark.seconds(), last_dump_head_mark.pts());
        LDEBUG(ZCSTR("avpacket"), ZConstString(msg, strlen(msg)));

        for (auto c = deq.begin(); c != deq.end(); ++c)
        {
            seq_key_t& k = (*c).first;
            if (k.pts() > last_dump_tail_mark.pts() && k.pts() < key.pts())
            {
                pfile->write((*c).second->getAVPacket());
            }
        }
        deq.clear();
        seq.erase(it);
    }

    int sz = seq.size();
    int cnt = 0;
    for (it = seq.begin(); cnt < (sz - 1); it = seq.begin(), ++cnt)
    {
        mark = (*it).first;
        last_dump_head_mark = mark;
        if ( (key.first -  mark.first) < 1000.0 * ZMBAQ_PACKETS_BUFFERING_SECONDS)
        {
            packet_list_t& deq ((*it).second);
            snprintf(msg, sizeof(msg),
                    "file: %s frames sequence #%d with %lu frames. Head timesec: %lf, pts: %lu",
                    pfile->path().data(), cnt, deq.size(), mark.first, mark.second);
            LDEBUG(ZCSTR("avpacket"), ZConstString(msg, strlen(msg)));
            for (auto c = deq.begin(); c != deq.end(); ++c)
            {
                pfile->write((*c).second->getAVPacket());
                last_dump_tail_mark = (*c).first;
            }
            deq.clear();
        }
        seq.erase(it);
    }
    // 1 last sequence left in the seq
}

void PacketsPocket::push(AVPacket* pkt, SHP(ZMB::FFileWriterBase) pfile,
                         const AVRational& time_base)
{
    auto key = seq_key_t(laptime.elapsed(), pkt->pts);
    marked_pkt_t mpkt (key, std::make_shared<av::Packet>(pkt));

    if (pkt->flags & AV_PKT_FLAG_KEY)
    {
        packet_list_t deq;
        deq.push_back(mpkt);
        seq.push_back(seq_item_t(key, deq));
    }
    else
    {
        auto it = seq.rbegin();
        if (it != seq.rend())
        {
            //get the packet_list_t:
            (*it).second.push_back(mpkt);
        }
        //else we skip packets until first key-frame comes
    }
    if (seq.size() > 2)
    {
        dump_packets(key, pfile);
    }


    prev_pts = pkt->pts;

}
//-----------------------------------------------------------------

//Inits them once within the program:
/*extern symbol*/void init_ffmpeg_libs();

ffmpeg_rtp_encoder::ffmpeg_rtp_encoder()
{
    sws_ctx = NULL;
    f_ready = false;
    oc = 0;
    d_rtp_port = 0;
    video_st = 0;
    video_codec = 0;
    video_is_eof = 0;
    frame = 0;
    frame_count = 0;
    dst_picture = new AVPicture;
    memset(dst_picture, 0x00, sizeof(AVPicture));
    sidedata = nullptr;

    dict = 0;
}

ffmpeg_rtp_encoder::~ffmpeg_rtp_encoder()
{
    av_dict_free(&dict);
    delete dst_picture;
}

glm::ivec2 ffmpeg_rtp_encoder::default_enc_frame_size()
{
   return glm::ivec2(1280, 720);
}

ZMBCommon::MByteArray ffmpeg_rtp_encoder::get_sprop_parameters(glm::ivec2 encoded_frame_size)
{
    static u_int16_t rnd_port = 19999;
    static std::mutex m;
    m.lock();
    create_stream("127.0.0.1", rnd_port++,
                  encoded_frame_size.x, encoded_frame_size.y);

    AVCodecContext* ctx = video_st->codec;
    char* sprop = extradata2psets(ctx);
    ZMBCommon::MByteArray props = ZConstString(sprop);
    if (nullptr != sprop)
        av_free (sprop);

    destroy_stream();
    ZMBCommon::MByteArrayList msg;
    msg << __PRETTY_FUNCTION__;
    msg << "*** sample S-prop parameters of a RTP stream: ***";
    msg << props;
    AINFO(msg);
    m.unlock();
    return props;

}

/** Encode frame and send it.*/
bool ffmpeg_rtp_encoder::push_data(const u_int8_t* data, glm::ivec2 data_size, int data_format)
{
    if (!f_ready || oc == nullptr)
        return false;

    AVCodecContext *c = video_st->codec;
    if (!sws_ctx)
    {
        pock.laptime.lapTime();
        sws_ctx = sws_getCachedContext( sws_ctx, data_size.x, data_size.y, (AVPixelFormat)data_format,
                                  c->width, c->height, AV_PIX_FMT_YUV420P,
                                  sws_flags, NULL, NULL, NULL);
        assert(NULL != sws_ctx);
    }
    AVPicture inp;
    memset(&inp, 0x00, sizeof(inp));
    avpicture_fill (&inp, data, (AVPixelFormat)data_format, data_size.x, data_size.y);
    sws_scale(sws_ctx,
              (const uint8_t * const *)inp.data, inp.linesize,
              0, data_size.y, dst_picture->data, dst_picture->linesize);

    return true;
}

bool ffmpeg_rtp_encoder::send_data()
{
    if (!f_ready || oc == nullptr)
        return false;

    if (NULL == frame || NULL == dst_picture->data[0] || NULL == video_st )
        return false;

    flush = 0;
    if (video_st && !video_is_eof)
    {
        /* Compute current audio and video time. */
        video_time = video_st->cur_dts * av_q2d(video_st->time_base);

        if (!flush && video_time >= STREAM_DURATION)
        {
            flush = 1;
        }

        /* write interleaved audio and video frames */
        if (video_st && !video_is_eof)
        {
            write_video_frame (oc, video_st, flush);
            return true;
        }
    }
    return false;
}
//----------------------------------------------------------------------

/** rescale output packet timestamp values from codec to stream timebase */
static void pkt_time_rescale(const AVRational *time_base, AVStream *st, AVPacket* pkt)
{
    pkt->pts = av_rescale_q_rnd(pkt->pts, *time_base, st->time_base,
                                (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
    pkt->dts = av_rescale_q_rnd(pkt->dts, *time_base, st->time_base,
                                (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
    pkt->duration = av_rescale_q(pkt->duration, *time_base, st->time_base);
    pkt->stream_index = st->index;
}

int ffmpeg_rtp_encoder::write_frame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt)
{
    pkt_time_rescale(time_base, st, pkt);
    pock.push(pkt, p_file_writer, *time_base);

    int res = av_interleaved_write_frame(fmt_ctx, pkt);
    av_free_packet(pkt);
    return res;
}


/**************************************************************/
bool ffmpeg_rtp_encoder::send_packet(AVPacket* pkt)
{
    /* If size is zero, it means the image was buffered. */
    int ret = write_frame(oc, &(video_st->codec->time_base), video_st, pkt);
    return 0 == ret;
}

bool ffmpeg_rtp_encoder::write_video_frame(AVFormatContext *oc, AVStream *st, int flush)
{
    int ret;
    AVCodecContext *c = st->codec;
    auto check_if_good_ret = [&]( ) mutable -> bool{
        if (ret < 0)
        {
            ZMBCommon::MByteArrayPtr msg = make_mbytearray(__PRETTY_FUNCTION__);
            (*msg) += ZCSTR(" write failed.");
            LERROR("FF", msg);
            ++bad_frame_count;
            if (nullptr != cb_failed) cb_failed(this, msg);
            return false;
        }
        ++frame_count;
        return true;
    };


    if (oc->oformat->flags & AVFMT_RAWPICTURE && !flush) {
        /* Raw video case - directly store the picture in the packet */
        AVPacket pkt;
        av_init_packet(&pkt);

        pkt.flags        |= AV_PKT_FLAG_KEY;
        pkt.stream_index  = st->index;
        pkt.data          = dst_picture->data[0];
        pkt.size          = sizeof(AVPicture);

        ret = av_interleaved_write_frame(oc, &pkt);
    } else {
        AVPacket pkt;
        memset(&pkt, 0x00, sizeof(pkt));
        int got_packet;
        av_init_packet(&pkt);

        /* encode the image */
        frame->pts = frame_count;
        ret = avcodec_encode_video2(c, &pkt, flush ? NULL : frame, &got_packet);

        if (ret >= 0)
        {
            /* If size is zero, it means the image was buffered. */

            if (got_packet)
            {
                ret = write_frame(oc, &c->time_base, st, &pkt);
            } else {
                if (flush) video_is_eof = 1;
                ret = 0;
            }
        }
    }

    return check_if_good_ret();
}

void ffmpeg_rtp_encoder::close_video(AVStream *st)
{
    avcodec_close(st->codec);
    av_free(dst_picture->data[0]);
    av_frame_free(&frame);
}

/**************************************************************/

void ffmpeg_rtp_encoder::open_video(AVCodec *codec, AVStream *st)
{
    int ret;
    AVCodecContext *c = st->codec;
    c->rtp_payload_size = 1200;
    c->profile = FF_PROFILE_H264_BASELINE;
    c->flags |= CODEC_FLAG_GLOBAL_HEADER;

    /* open the codec */
    ret = avcodec_open2(c, codec, NULL);
    assert(ret >= 0);

    /* allocate and init a re-usable frame */
    frame = av_frame_alloc();
    assert(NULL != frame);

    frame->format = c->pix_fmt;
    frame->width = c->width;
    frame->height = c->height;

    /* Allocate the encoded raw picture. */
    ret = avpicture_alloc(dst_picture, c->pix_fmt, c->width, c->height);
    assert(ret >= 0);

    /* copy data and linesize picture pointers to frame */
    memcpy(frame->data, dst_picture->data, sizeof(dst_picture->data));
    memcpy(frame->linesize, dst_picture->linesize, sizeof(dst_picture->linesize));
}

AVStream* ffmpeg_rtp_encoder::make_vstream_w_defaults(AVFormatContext* afctx, AVCodec** codec) const
{
    AVStream* st = avformat_new_stream(afctx, *codec);

    if (NULL == st)
    {
        ZMBCommon::MByteArrayPtr msg = make_mbytearray(__PRETTY_FUNCTION__);
        (*msg) += "avformat_new_stream() returned NULL";
        if (0 != cb_failed) cb_failed(this, msg);
        return NULL;
    }
//    st->id = oc->nb_streams-1;
    st->time_base.den = ZMBAQ_ENCODER_DEFAULT_FPS;
    st->time_base.num = 1;
    st->r_frame_rate.num = ZMBAQ_ENCODER_DEFAULT_FPS;
    st->r_frame_rate.den = 1;
    st->start_time = AV_NOPTS_VALUE;
    return st;
}


/* Add an output stream. */
AVStream * ffmpeg_rtp_encoder::add_stream(AVFormatContext* fmt_ctx, AVCodec **codec, int codec_id) const
{

    /* find the encoder */
    *codec = avcodec_find_encoder((AVCodecID)codec_id);
    if (NULL == (*codec))
    {
        ZMBCommon::MByteArrayPtr msg = make_mbytearray(__PRETTY_FUNCTION__);
        (*msg) += "avcodec_find_encoder() returned NULL";
        if (0 != cb_failed) cb_failed(this, msg);
        return NULL;
    }
    AVStream *st = make_vstream_w_defaults(fmt_ctx, codec);
    AVCodecContext *c = st->codec;
    avcodec_get_context_defaults3(c, *codec);

    c->codec_id = (enum AVCodecID )codec_id;
    set_vcodec_defaults(st->codec, st, d_encod_size);

    /* Some formats want stream headers to be separate. */
    if (fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= CODEC_FLAG_GLOBAL_HEADER;

    return st;
}

void ffmpeg_rtp_encoder::set_vcodec_defaults(AVCodecContext* c, AVStream* st, glm::ivec2 encod_size) const
{
    c->coder_type = FF_CODER_TYPE_AC;
    c->bit_rate = ZMBAQ_ENCODER_BITRATE;
    /* Resolution must be a multiple of two. */
    c->width    = encod_size.x;
    c->height   = encod_size.y;
    /* timebase: This is the fundamental unit of time (in seconds) in terms
         * of which frame timestamps are represented. For fixed-fps content,
         * timebase should be 1/framerate and timestamp increments should be
         * identical to 1. */
    c->time_base.den = ZMBAQ_ENCODER_DEFAULT_FPS;
    c->time_base.num = 1;
    /* emit one intra frame every (x)twelve frames at most */
    c->gop_size      = ZMBAQ_ENCODER_GOP_SIZE;
    c->pix_fmt       = STREAM_PIX_FMT;
    c->sample_aspect_ratio.num = st->sample_aspect_ratio.num;
    c->sample_aspect_ratio.den = st->sample_aspect_ratio.den;

    if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
        /* just for testing, we also add B frames */
        c->max_b_frames = 2;
    }
    if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
        /* Needed to avoid using macroblocks in which some coeffs overflow.
             * This does not happen with normal video, it just happens here as
             * the motion of the chroma plane does not match the luma plane. */
        c->mb_decision = 2;
    }

}

struct TmpCSFuncCleaner
{
    typedef std::function<void(const ffmpeg_rtp_encoder*, ZMBCommon::MByteArrayPtr)> fail_fn_t;
    fail_fn_t cb_failed;

    TmpCSFuncCleaner(const ffmpeg_rtp_encoder* pff = 0, fail_fn_t fail_fn = nullptr)
        : ff(pff), cb_failed(fail_fn)
    {
       msg.str = __msg;
       msg.len = sizeof(__msg);
       msg.fill(0);
       status = 0;
    }

    ~TmpCSFuncCleaner()
    {
        if (status < 0 && nullptr != cb_failed )
        {
           cb_failed(ff, make_mbytearray(msg.to_const()));
           assert(false);
        }
    }

    TmpCSFuncCleaner& operator << (ZConstString out)
    {
        char* e = 0;
        msg.read(&e, out);
        msg.len = e - msg.begin();
        status = -1;
        return *this;
    }

    const ffmpeg_rtp_encoder* ff;
    char __msg[128];
    ZUnsafeBuf msg;
    int status;

};

/** TODO: THIS FUCK DOES NOT WORK. */
int ffmpeg_rtp_encoder::create_stream(const AVFormatContext* src_ctx,
                                      ZConstString rtp_ip, u_int16_t rtp_port)
{
    destroy_stream();

    d_rtp_ip = rtp_ip;
    d_rtp_port = rtp_port;


    /* Initialize libavcodec, and register all codecs and formats. */
    init_ffmpeg_libs();

    /* allocate the output media context */
    oc = avformat_alloc_context();
    src_ctx->oformat;
    AVOutputFormat* rtp_format = av_guess_format("rtp", NULL, NULL);
    rtp_format->video_codec = AV_CODEC_ID_H264 ;
    rtp_format->audio_codec = AV_CODEC_ID_NONE;

    oc->oformat = rtp_format;
    oc->start_time = AV_NOPTS_VALUE;
    oc->start_time_realtime = AV_NOPTS_VALUE;

    av_dict_set (&oc->metadata, "title", "EMPTY", 0);
    snprintf(oc->filename, 13 + rtp_ip.size(),
             "rtp://%s:%u", rtp_ip.begin(), rtp_port);


    fmt = oc->oformat;

    /* Add the audio and video streams using the default format codecs
     * and initialize the codecs. */
    video_st = NULL;

   TmpCSFuncCleaner cleaner;

    /* Now that all the parameters are set, we can open the audio and
     * video codecs and allocate the necessary encode buffers. */
   uint32_t i  = 0;
   for (i = 0; i < src_ctx->nb_streams; ++i)
   {
       if (src_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
       {
           AVStream* in_stream = src_ctx->streams[i];
           video_st = avformat_new_stream(oc, in_stream->codec->codec);

           int errnum = 0;
           if ((errnum = avcodec_copy_context(video_st->codec, in_stream->codec)) < 0)
           {
               cleaner << ZCSTR("avcodec_copy_context()");
               return cleaner.status;
           }

           video_st->time_base           = in_stream->time_base;
           video_st->codec->codec_id     = in_stream->codec->codec_id;
           video_st->sample_aspect_ratio = in_stream->sample_aspect_ratio;
           video_st->avg_frame_rate = in_stream->avg_frame_rate;
           d_encod_size.x = (video_st->codec->width);
           d_encod_size.y = (video_st->codec->height);

           if (rtp_format->flags & AVFMT_GLOBALHEADER)
           {
               video_st->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
           }
           break;
       }
   }

    if (nullptr == video_st || i == src_ctx->nb_streams)
    {
        cleaner << ZCSTR("FFmpeg add_stream() method failed.");
        return cleaner.status;
    }

    /* open the output file, if needed */
    if (!(fmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&oc->pb, oc->filename, AVIO_FLAG_WRITE);
        if (ret < 0)
        {
            cleaner << ZCSTR("FFmpeg avio_open() failed.");
            return cleaner.status;
        }
    }

    /* Write the stream header, if any. */
    ret = avformat_write_header(oc, NULL);
    if (ret < 0)
    {
        cleaner << ZCSTR("FFmpeg avformat_write_header() failed");
        return cleaner.status;
    }

    //report RTP settings if anyone is interested.
    auto rtp = make_rtp_settings();
    if (nullptr != cb_stream_created)
        cb_stream_created(this, rtp);

    f_ready = true;
    return 0;
}

int ffmpeg_rtp_encoder::create_stream(ZConstString rtp_ip, u_int16_t rtp_port,
                                      int enc_width, int enc_height)
{
    if (nullptr != oc && d_rtp_ip.get_const_str() == rtp_ip
            && d_encod_size == glm::ivec2(enc_width, enc_height))
    {//case already created
        return 0 ;
    }
    else
    {
        destroy_stream();
    }
    d_rtp_ip = rtp_ip;
    d_rtp_port = rtp_port;
    d_encod_size = glm::ivec2(enc_width, enc_height);

    /* Initialize libavcodec, and register all codecs and formats. */
    init_ffmpeg_libs();

    /* allocate the output media context */
    oc = avformat_alloc_context();
    AVOutputFormat* rtp_format = av_guess_format("rtp", NULL, NULL);
    rtp_format->video_codec = AV_CODEC_ID_H264 ;
    rtp_format->audio_codec = AV_CODEC_ID_NONE;
    oc->oformat = rtp_format;
    oc->start_time = AV_NOPTS_VALUE;
    oc->start_time_realtime = AV_NOPTS_VALUE;

    av_dict_set (&oc->metadata, "title", "EMPTY", 0);
    snprintf(oc->filename, 13 + rtp_ip.size(),
             "rtp://%s:%u", rtp_ip.begin(), rtp_port);


    fmt = oc->oformat;

    /* Add the audio and video streams using the default format codecs
     * and initialize the codecs. */
    video_st = NULL;

    if (fmt->video_codec != AV_CODEC_ID_NONE)
        video_st = add_stream(oc, &video_codec, fmt->video_codec);
    /* Now that all the parameters are set, we can open the audio and
     * video codecs and allocate the necessary encode buffers. */
    if (video_st)
    {
        open_video(video_codec, video_st);
    }
    else
    {
        if (0 != cb_failed) cb_failed(this, make_mbytearray("FFmpeg add_stream() method failed."));
        return -1;
    }

    /* open the output file, if needed */
    if (!(fmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&oc->pb, oc->filename, AVIO_FLAG_WRITE);
        if (ret < 0)
        {
            if (0 != cb_failed) cb_failed(this, make_mbytearray("FFmpeg avio_open() failed."));
            return -1;
        }
    }

    /* Write the stream header, if any. */
    ret = avformat_write_header(oc, NULL);
    if (ret < 0)
    {
        if (0 != cb_failed) cb_failed(this, make_mbytearray("FFmpeg avformat_write_header() failed"));
        return -1;
    }

    if (nullptr != video_st)
    {
        //copy S-prop parameters:
        auto rtp = make_rtp_settings();
        if (nullptr != cb_stream_created) cb_stream_created(this, make_rtp_settings());
    }

    f_ready = true;
    return 0;
}


std::shared_ptr<Json::Value> ffmpeg_rtp_encoder::make_rtp_settings()
{
    SHP(Json::Value) rtp_p = std::make_shared<Json::Value>();
    Json::Value& rtp(*rtp_p);

    if (nullptr != video_st)
    {
        char* sprop = extradata2psets(video_st->codec);
        sprop_parameters = ZConstString(sprop);
        if (nullptr != sprop)
            av_free (sprop);
        rtp["sprop"] = (sprop_parameters.data());
    }

    rtp["name"] = d_rtp_ip.c_str();
    Poco::DateTime dt;
    Poco::Timestamp ts =  dt.utcTime();
    u_int64_t cmsec =  ts.epochMicroseconds();
    cmsec /= 1000;
    cmsec += 2208988800UL;
    rtp["v"] = (int)0;
    rtp["sid"] = (tm.elapsed());
    rtp["port"] = ((int)d_rtp_port);
    rtp["ip"] = (d_rtp_ip.c_str());
    return rtp_p;
}

class ZCArray16 : public std::array<ZConstString, 16>
{
public:
    ZCArray16() : n_items(0)
    { }
    void append(const ZConstString& item)
    {
        at(n_items) = item;
        ++n_items;
    }
    u_int32_t n_items;
};

ZMBCommon::MByteArrayPtr ffmpeg_rtp_encoder::make_rtp_settings_str(const Json::Value& jo)
{
   ZMBCommon::MByteArrayPtr rtp_p = make_mbytearray();
   ZMBCommon::MByteArray& rtp(*rtp_p);
   rtp.reserve(256);

   ZCArray16 strs;
   strs.append(ZCSTR("v=0\r\n"));

   double sid_lf = jo["sid"].asDouble();
   sid_lf *= 1000;
   u_int64_t s = sid_lf;
   char sid[64]; memset(sid, 0, sizeof(sid));
   snprintf(sid, 64, " %llu %llu ", s, s);


   strs.append(ZCSTR("o="));
   strs.append(ZConstString(ZCSTR("name"), &jo));
   strs.append(ZConstString(sid, strlen(sid)));
   strs.append(ZCSTR(" IN IP4 "));

   if (jo.isMember("rtp"))
   {
       strs.append(ZConstString(ZCSTR("rtp"), &jo));
   }
   strs.append(ZCSTR(" \r\n"));
   strs.append(ZCSTR("s=media session\r\n"));

   auto ba = jo["ip"].asString();
   char* _s = (char*)alloca(ba.size() + 13);
   snprintf(_s, 13, "c=IN IP4 %s \r\n", ba.data());
   strs.append(ZConstString(_s, ba.size() + 13));

   static const char __temp[] = "m=video %d RTP/AVP 96\r\na=rtpmap:96 H264/90000\r\n";
   _s = (char*)alloca(sizeof(__temp) + 4);
   snprintf(_s, sizeof(__temp) + 4, __temp, jo["port"].asInt());
   strs.append(ZConstString(_s, strlen(_s)));

   strs.append(ZCSTR("a=fmtp:96 packetization-mode=1"));
   if (jo.isMember("sprop"))
   {
       strs.append(ZConstString(ZCSTR("sprop"), &jo));
   }
   strs.append("\r\n");


   return rtp_p;
}

ZConstString ffmpeg_rtp_encoder::get_sprop() const
{
    return sprop_parameters.get_const_str();
}

void ffmpeg_rtp_encoder::destroy_stream()
{

    f_ready = false;
    /* Write the trailer, if any. The trailer must be written before you
     * close the CodecContexts open when you wrote the header; otherwise
     * av_write_trailer() may try to use memory that was freed on
     * av_codec_close(). */
    if (nullptr != oc)
    {

        if (nullptr != oc->streams)
        {
            av_write_trailer(oc);
        }

        /* Close each codec. */
        if (video_st)
        {
            close_video(video_st);
        }

        if (nullptr != fmt && !(fmt->flags & AVFMT_NOFILE))
        {
            /* Close the output file. */
            avio_close(oc->pb);
            fmt = 0;
        }

        /* free the stream */
        avformat_free_context(oc);
    }

    if (sws_ctx)
    {
        sws_freeContext(sws_ctx);
    }

    sws_ctx = NULL;
    f_ready = false;
    oc = 0;
    video_st = 0;
    video_codec = 0;
    video_is_eof = 0;
    frame = 0;
    frame_count = 0;
    memset(dst_picture, 0x00, sizeof(AVPicture));
}

bool ffmpeg_rtp_encoder::valid() const
{
   return f_ready && (nullptr != oc);
}
//=============================================================================
extern "C"
{

#include "libavutil/parseutils.h"
#include "libavutil/base64.h"

char *ff_data_to_hex(char *buff, const uint8_t *src, int s, int lowercase);

#define MAX_PSET_SIZE 1024
#define MAX_EXTRADATA_SIZE ((INT_MAX - 10) / 2)

static const uint8_t *ff_avc_find_startcode_internal(const uint8_t *p, const uint8_t *end)
{
    const uint8_t *a = p + 4 - ((intptr_t)p & 3);

    for (end -= 3; p < a && p < end; p++) {
        if (p[0] == 0 && p[1] == 0 && p[2] == 1)
            return p;
    }

    for (end -= 3; p < end; p += 4) {
        uint32_t x = *(const uint32_t*)p;
//      if ((x - 0x01000100) & (~x) & 0x80008000) // little endian
//      if ((x - 0x00010001) & (~x) & 0x00800080) // big endian
        if ((x - 0x01010101) & (~x) & 0x80808080) { // generic
            if (p[1] == 0) {
                if (p[0] == 0 && p[2] == 1)
                    return p;
                if (p[2] == 0 && p[3] == 1)
                    return p+1;
            }
            if (p[3] == 0) {
                if (p[2] == 0 && p[4] == 1)
                    return p+2;
                if (p[4] == 0 && p[5] == 1)
                    return p+3;
            }
        }
    }

    for (end += 3; p < end; p++) {
        if (p[0] == 0 && p[1] == 0 && p[2] == 1)
            return p;
    }

    return end + 3;
}

static const uint8_t *ff_avc_find_startcode(const uint8_t *p, const uint8_t *end)
{
    const uint8_t *out= ff_avc_find_startcode_internal(p, end);
    if(p<out && out<end && !out[-1]) out--;
    return out;
}

char *ff_data_to_hex(char *buff, const uint8_t *src, int s, int lowercase)
{
    int i;
    static const char hex_table_uc[16] = { '0', '1', '2', '3',
                                           '4', '5', '6', '7',
                                           '8', '9', 'A', 'B',
                                           'C', 'D', 'E', 'F' };
    static const char hex_table_lc[16] = { '0', '1', '2', '3',
                                           '4', '5', '6', '7',
                                           '8', '9', 'a', 'b',
                                           'c', 'd', 'e', 'f' };
    const char *hex_table = lowercase ? hex_table_lc : hex_table_uc;

    for (i = 0; i < s; i++) {
        buff[i * 2]     = hex_table[src[i] >> 4];
        buff[i * 2 + 1] = hex_table[src[i] & 0xF];
    }

    return buff;
}

static char* extradata2psets(AVCodecContext *c)
{
    char *psets = nullptr, *p = nullptr;
    const uint8_t *r = nullptr;
    static const char pset_string[] = "; sprop-parameter-sets=";
    static const char profile_string[] = "; profile-level-id=";
    uint8_t *orig_extradata = NULL;
    int orig_extradata_size = 0;
    const uint8_t *sps = NULL, *sps_end;

    if (c->extradata_size > MAX_EXTRADATA_SIZE)
    {
        av_log(c, AV_LOG_ERROR, "Too much extradata!\n");

        return NULL;
    }
    if (nullptr == c->extradata)
    {
        av_log(c, AV_LOG_WARNING, "Stream extradata not found!\n");
        return nullptr;
    }

    if (c->extradata[0] == 1)
    {
        uint8_t *dummy_p;
        int dummy_int;
        AVBitStreamFilterContext *bsfc = av_bitstream_filter_init("h264_mp4toannexb");

        if (!bsfc)
        {
            av_log(c, AV_LOG_ERROR, "Cannot open the h264_mp4toannexb BSF!\n");

            return NULL;
        }

        orig_extradata_size = c->extradata_size;
        orig_extradata = (u_int8_t*)av_mallocz(orig_extradata_size +
                                             FF_INPUT_BUFFER_PADDING_SIZE);
        if (!orig_extradata)
        {
            av_bitstream_filter_close(bsfc);
            return NULL;
        }
        memcpy (orig_extradata, c->extradata, orig_extradata_size);
        av_bitstream_filter_filter(bsfc, c, NULL, &dummy_p, &dummy_int, NULL, 0, 0);
        av_bitstream_filter_close(bsfc);
    }

    psets = (char*)av_mallocz(MAX_PSET_SIZE);
    if (psets == NULL)
    {
        av_log(c, AV_LOG_ERROR, "Cannot allocate memory for the parameter sets.\n");
        av_free(orig_extradata);
        return NULL;
    }
    memcpy(psets, pset_string, strlen(pset_string));
    p = psets + strlen(pset_string);
    r = ff_avc_find_startcode(c->extradata, c->extradata + c->extradata_size);

    while (r < c->extradata + c->extradata_size)
    {
        const uint8_t *r1 = nullptr;
        uint8_t nal_type = 0;

        while ( ! *(r++))
        {
            //cycle to the end..
        }

        nal_type = *r & 0x1f;
        r1 = ff_avc_find_startcode(r, c->extradata + c->extradata_size);
        if (nal_type != 7 && nal_type != 8)
        {
            /* Only output SPS and PPS */
            r = r1;
            continue;
        }
        if (p != (psets + strlen(pset_string)))
        {
            *p = ',';
            p++;
        }
        if (!sps)
        {
            sps = r;
            sps_end = r1;
        }
        if (av_base64_encode(p, MAX_PSET_SIZE - (p - psets), r, r1 - r) == NULL)
        {
            av_log(c, AV_LOG_ERROR, "Cannot Base64-encode %ld %ld!\n", MAX_PSET_SIZE - (p - psets), r1 - r);
            av_free(psets);
            return NULL;
        }
        p += strlen(p);
        r = r1;
    }

    if (sps && sps_end - sps >= 4)
    {
        memcpy(p, profile_string, strlen(profile_string));
        p += strlen(p);
        ff_data_to_hex(p, sps + 1, 3, 0);
        p[6] = '\0';
    }
    if (orig_extradata)
    {
        av_free(c->extradata);
        c->extradata      = orig_extradata;
        c->extradata_size = orig_extradata_size;
    }

    return psets;
}//extradata2psets

}//extern "C"

SHP(ZMB::FFileWriterBase) ffmpeg_rtp_encoder::write_file(const ZConstString& fname)
{
    AINFO(ZCSTR(__PRETTY_FUNCTION__));
    if (valid())
    {
        p_file_writer = std::make_shared<ZMB::FFileWriter>(oc, fname);
    }
    else
    {
        LINFO(ZCSTR("ALL"), ZCSTR("write_file FAIL: stream is not ready yet."));
    }
    return p_file_writer;
}

void ffmpeg_rtp_encoder::stop_file()
{
    AINFO(ZCSTR(__PRETTY_FUNCTION__));
    if (nullptr != p_file_writer)
    {
        p_file_writer->close();
    }
    else
    {
        AINFO(ZCSTR("stop_file FAIL: has not started."));
    }
}

ZConstString ffmpeg_rtp_encoder::rtp_ip() const
{
   return d_rtp_ip.get_const_str();
}


SHP(ZMB::FFileWriterBase) ffmpeg_rtp_encoder::cur_file() const
{
   return p_file_writer;
}
