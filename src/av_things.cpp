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


#include "av_things.h"
#include <iostream>
//#include <QImage>
//#include "sws_imgscaler.h"
#include "mbytearray.h"
#include "logservice.h"
#include <pthread.h>

extern "C" {
    #include <signal.h>
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavutil/avutil.h>
    #include <libavutil/frame.h>
    #include <libswscale/swscale.h>

    #include <pthread.h>
    #include <assert.h>
}

pthread_once_t av_once_control = PTHREAD_ONCE_INIT;
static pthread_mutex_t g_avcodec_mutex = PTHREAD_MUTEX_INITIALIZER;


static std::string averror_to_string(int errnum)
{
    std::string buf(1024, 0);
    av_strerror(errnum, &buf[0], buf.size());
    return buf;
}

static void init_ffmpeg()
{
    avcodec_register_all();
    avformat_network_init();
    av_register_all();
}

void init_ffmpeg_libs()
{
    pthread_once(&av_once_control, &init_ffmpeg);
}

struct av_things::impl
{

    bool m_is_open;
    int v_stream_idx;
    AVFrame* av_frame;

    //contais copy of pointers from av_frame
    AVPicture yuv_picture;

    AVCodecContext* av_codec_ctx;
    AVFormatContext* av_format_ctx;

    AVCodec* av_codec;

    av_things* p_master;

    //image scaler:
//    SwsImgScaler scaler;

//    //qimg pointer set after scaling:
//    SHP(QImage) qimg;
    glm::ivec2 output_size_limit;


    impl(av_things* master)
    {
        //be carefull with such things: (ZEROFILL THIS)

        m_is_open = false;
        p_master = master;
        av_codec_ctx = nullptr;
        av_format_ctx = nullptr;
        av_codec = nullptr;
        av_frame = nullptr;
        v_stream_idx = 0;

        init_ffmpeg_libs();

//        connect(&scaler, &SwsImgScaler::sig_error, p_master, &av_things::sig_sws_error);
    }

    ~impl()
    {
        close();
    }

    void close()
    {
        if( !m_is_open)
            return;

        if (av_frame)
        {
            av_frame_free(&av_frame);
        }

        if (av_codec_ctx)
            avcodec_close(av_codec_ctx);

        if (av_format_ctx != 0)
            avformat_close_input(&av_format_ctx);

        m_is_open = false;
    }

    void operator<<(const ZConstString& msg_dump)
    {
        std::cerr << msg_dump.begin();
        AERROR(msg_dump);
    }

    int get_video_stream_and_codecs()
    {
        v_stream_idx = -1;
        for (unsigned i = 0; i < av_format_ctx->nb_streams && v_stream_idx == -1; ++i)
        {
            /** TODO: add audio support here.**/
            if (av_format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
            {
                v_stream_idx = i;
                break;
            }
        }

        if(v_stream_idx == -1 || v_stream_idx == av_format_ctx->nb_streams)
        {
            *this << ZCSTR("Can not find video line \n");
            return -1;
        }
        else
        {
            // get a pointer to the codec context for the video stream
            av_codec_ctx = av_format_ctx->streams[v_stream_idx]->codec;
        }

        // find the decoder for the video stream
        av_codec = avcodec_find_decoder(av_codec_ctx->codec_id);

        if (av_codec == NULL)
        {
            *this << ZCSTR("Error! decoder not found \n");
            return -1;
        }

        int err = avcodec_open2(av_codec_ctx, av_codec, NULL);
        if(err != 0)
        {
            std::string error_m = "Error! Can open codec: " + averror_to_string(err) + "\n";
            *this << ZConstString(error_m.data(), error_m.size());
            return err;
        }
        else
        {

            //try to allocate frame
            av_frame = av_frame_alloc();
            if (!av_frame)
            {
                *this << "Error! Failed to alloc frame: \n";
                return -1;
            }
        }

        m_is_open = true;

        return v_stream_idx;
    }

    AVFrame* decode_frame(int& got_picture_indicator, AVPacket* pkt)
    {
        if(pkt->stream_index == v_stream_idx)
        {
            got_picture_indicator = 0;
            int res = avcodec_decode_video2(av_codec_ctx, av_frame, &got_picture_indicator, pkt);
            if (res < 0)
            {
//                AERROR(averror_to_string(res));
                p_master->reemit_ff_error(res);
            }
            else
            {
                if (got_picture_indicator)
                {
                    memcpy(yuv_picture.data, av_frame->data, sizeof(av_frame->data));
                    memcpy(yuv_picture.linesize, av_frame->linesize, sizeof(av_frame->linesize));

                    glm::ivec2 img_size(av_codec_ctx->width, av_codec_ctx->height);
                    if (output_size_limit.x > 0 && output_size_limit.y > 0)
                    {
                        img_size.x = (ZMB::min<int>(img_size.x, output_size_limit.x));
                        img_size.y = (ZMB::min<int>(img_size.y, output_size_limit.y));

                        output_size_limit = img_size;
                    }
//                    qimg = scaler.scale(&yuv_picture, glm::ivec2(av_codec_ctx->width, av_codec_ctx->height), (int)av_codec_ctx->pix_fmt, img_size);
                }
            }
        }
        return av_frame;
    }
};

void av_things::limit_output_img_size(glm::ivec2 size)
{
    pv->output_size_limit = size;
}

av_things::av_things()
{
    pv = std::make_shared<impl>(this);
}

av_things::~av_things()
{

}

void av_things::reemit_ff_error(int errl)
{
//TODO: re-emit error.
}

int av_things::open(const ZConstString& url)
{
    int err = avformat_open_input(&pv->av_format_ctx, url.begin(), 0, 0);
    if(err != 0)
    {
        auto e = averror_to_string(err);
	*pv << ZCSTR("Error! Can not find stream info: ");
	*pv << ZConstString(e.data(), e.size())
            << ZCSTR("\n requested URL: ")
            << url;
        return err;
    }
    else
    {
        err = avformat_find_stream_info(pv->av_format_ctx, NULL);
    }

    return err;
}

void av_things::close()
{
    pv->close();
}

bool av_things::is_open() const
{
   return pv->m_is_open;
}

int av_things::get_video_stream_and_codecs()
{
    return pv->get_video_stream_and_codecs();
}

pthread_mutex_t& av_things::mutex()
{
   return g_avcodec_mutex;
}

void av_things::read_play()
{
    av_read_play(pv->av_format_ctx);
}

int av_things::read_frame(AVPacket* pkt)
{
    assert(pkt != 0);
    return av_read_frame(pv->av_format_ctx, pkt);
}

AVFrame* av_things::decode_frame(int& got_picture_indicator, AVPacket* pkt)
{
    return pv->decode_frame(got_picture_indicator, pkt);
}

AVPicture* av_things::frame(int& av_pix_fmt, glm::ivec2& dim)
{
    av_pix_fmt = pv->av_frame->format;
    dim = glm::ivec2(pv->av_frame->width, pv->av_frame->height);
    //copy pointer arrays:
    memcpy(pv->yuv_picture.data, pv->av_frame->data, sizeof(pv->av_frame->data));
    memcpy(pv->yuv_picture.linesize, pv->av_frame->linesize, sizeof(pv->av_frame->linesize));
    return &(pv->yuv_picture);
}

//u_int8_t* av_things::converted_frame(size_t& len) const
//{
//    if (nullptr != pv->qimg)
//    {
//        len = pv->qimg->byteCount();
//        return pv->qimg->bits();
//    }
//    len = 0;
//    return 0;
//}

//SHP(QImage) av_things::converted_frame() const
//{
//    return pv->qimg;
//}

int av_things::width() const
{
    return pv->av_codec_ctx->width;
}

int av_things::height() const
{
   return pv->av_codec_ctx->height;
}

//int av_things::bytes_per_line() const
//{
//   return pv->qimg->bytesPerLine();
//}

int av_things::vstream_index() const
{
   return pv->v_stream_idx;
}

double r2d(const AVRational& r)
{
    return r.num == 0 || r.den == 0 ? 0. : (double)r.num / (double)r.den;
}

double av_things::fps() const
{
    static const double eps_zero = 0.000025;
    double fps = r2d(pv->av_format_ctx->streams[pv->v_stream_idx]->r_frame_rate);

    if (fps < eps_zero)
    {
        fps = r2d(pv->av_format_ctx->streams[pv->v_stream_idx]->avg_frame_rate);
    }

    if (fps < eps_zero)
    {
        fps = 1.0 / r2d(pv->av_format_ctx->streams[pv->v_stream_idx]->codec->time_base);
    }
    return fps;
}

AVFormatContext* av_things::get_format_ctx() const
{
   return pv->av_format_ctx;
}

const AVRational* av_things::get_timebase() const
{
    assert(is_open());
    return &pv->av_format_ctx->streams[pv->v_stream_idx]->time_base;
}
