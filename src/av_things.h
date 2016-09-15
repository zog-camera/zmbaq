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


#ifndef AV_THINGS_H
#define AV_THINGS_H

#include <string>
#include "zmbaq_common/zmbaq_common.h"
#include <glm/vec2.hpp>

struct AVFrame;
struct AVPicture;
struct AVPacket;
struct AVFormatContext;
struct AVRational;

namespace ZMB {

using namespace ZMBCommon;

//Inits them once within the program:
void init_ffmpeg_libs();

/** Class wraps functions of ffmpeg to open streams.
 *  Usage example:
 * @code

    av_things myav;
    if(myav.open(argv[1]) != 0) { //die
        return -1;
    }
    if (myav.get_video_stream_and_codecs() < 0) { //die
        return -1;
    }
    myav.read_play();

    AVPacket pkt;
    for(;;)
    {
        AVFrame* yuv_frame = nullptr;
        if(myav.read_frame(&pkt) == 0)
        {
            int got_picture = 0;
            yuv_frame = myav.decode_frame(got_picture, &pkt);
            if(0 == got_picture)
            {
                int av_pix_fmt = 0; glm::ivec2 dim(0,0);
                AVPicture* pic = myav.frame(av_pix_fmt, dim);
                // ... some nasty things with data here....
            }
        }
    }
 * @endcode
*/
class av_things
{
public:
//    void sig_ff_error(int/*ffmpeg err code*/);
//    void sig_sws_error(SHP(MByteArray) msg);


    av_things();
    virtual ~av_things();

    /** Opens the URL and tries to find stream info.
     * @return 0 if OK, averror code otherwise(less than 0).*/
    int open(const ZConstString& url);

    void close();

    bool is_open() const;

    /** Gets video stream index, finds decoder and opens the codec,
     *  if any of those failed will return -1;
     *
     * @return -1 if failed or value greater than 0 if Okay.*/
    int get_video_stream_and_codecs();

    /** Start playing stream*/
    void read_play();

    /** Reads packets from source.
     * 	@return -1 if packet read failed */
    int read_frame(AVPacket* pkt);

    /** Decoded frame from packet.
     *  @param got_picture_indicator: > 0 if frame decoded.
     * 	@return pointer valid while av_things object exists,
     * 	decoded frame data shall be overwritten in next method call.
     *
  */
    AVFrame* decode_frame(int& got_picture_indicator, AVPacket* pkt);

    void limit_output_img_size(glm::ivec2 size);

    /** Get fps in seconds*/
    double fps() const;

    //video stream index
    int vstream_index() const;

    //decoded frame width
    int width() const;

    //decoded frame height
    int height() const;

    //single mutex for all instances
    static pthread_mutex_t& mutex();

    //required for FFileWriter
    AVFormatContext* get_format_ctx() const;

    const AVRational* get_timebase() const;

    void reemit_ff_error(int errl);

private:
    struct impl;
    std::shared_ptr<impl> pv;
};

}//namespace ZMB

#endif // AV_THINGS_H
